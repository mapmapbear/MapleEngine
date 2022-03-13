//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "Engine/Mesh.h"
#include "Engine/Core.h"

namespace maple 
{
	namespace MeshLoader
	{
		auto MAPLE_EXPORT load(const std::string& obj, std::unordered_map<std::string, std::shared_ptr<Mesh>>&)-> void;
	};

	class MAPLE_EXPORT ModelLoader
	{
	public:
		virtual auto load(const std::string& obj, const std::string& extension, std::unordered_map<std::string, std::shared_ptr<Mesh>>&)-> void = 0;
	};

	class MAPLE_EXPORT ModelLoaderFactory
	{
	public:
		ModelLoaderFactory();

		template<typename T> 
		auto addModelLoader() -> void;
		auto load(const std::string& obj, std::unordered_map<std::string, std::shared_ptr<Mesh>>&)-> void;

		inline auto& getSupportExtensions() const{
			return supportExtensions;
		}
	private:
		std::unordered_map<std::string, std::shared_ptr<ModelLoader>> loaders;
		std::unordered_set<std::string> supportExtensions;
	};

	template<typename T>
	inline auto ModelLoaderFactory::addModelLoader() -> void
	{
		auto loader = std::make_shared<T>();
		for (auto ext : T::EXTENSIONS) 
		{
			loaders.emplace(ext, loader);
			supportExtensions.emplace(ext);
		}
	}
};
