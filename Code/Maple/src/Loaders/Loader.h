//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Engine/Core.h"
#include "Engine/Mesh.h"

namespace maple
{
	class Skeleton;
	class Animation;

	namespace Loader
	{
		auto MAPLE_EXPORT load(const std::string &obj, std::vector<std::shared_ptr<IResource>> &out) -> void;
	};

	class MAPLE_EXPORT AssetsLoader
	{
	  public:
		virtual auto load(const std::string &fileName, const std::string &extension, std::vector<std::shared_ptr<IResource>> &out) const -> void{};

	  private:
	};

	class MAPLE_EXPORT AssetsLoaderFactory
	{
	  public:
		AssetsLoaderFactory();

		template <typename T>
		typename std::enable_if<std::is_base_of<AssetsLoader, T>::value, void>::type addModelLoader()
		{
			auto loader = std::make_shared<T>();
			for (auto ext : T::EXTENSIONS)
			{
				loaders.emplace(ext, loader);
				supportExtensions.emplace(ext);
			}
		}

		auto load(const std::string &obj, std::vector<std::shared_ptr<IResource>> &out) -> void;

		inline auto &getSupportExtensions() const
		{
			return supportExtensions;
		}

		template <typename T, typename... Args>
		auto emplace(const std::string &obj, Args &&... args) -> std::shared_ptr<T>;

		inline auto &getCache() const
		{
			return cache;
		}

	  private:
		std::unordered_map<std::string, std::shared_ptr<AssetsLoader>>           loaders;
		std::unordered_set<std::string>                                          supportExtensions;
		std::unordered_map<std::string, std::vector<std::shared_ptr<IResource>>> cache;
	};

	template <typename T, typename... Args>
	auto AssetsLoaderFactory::emplace(const std::string &obj, Args &&... args) -> std::shared_ptr<T>
	{
		auto iter = cache.find(obj);
		if (iter == cache.end())
		{
			auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
			cache[obj].emplace_back(ptr);
			return std::static_pointer_cast<T>(ptr);
		}
		return std::static_pointer_cast<T>(iter->second[0]);
	}
};        // namespace maple
