//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

extern "C"
{
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#include "Engine/Core.h"
#include "MetaFile.h"
#include <LuaBridge/LuaBridge.h>
#include <entt/entt.hpp>
#include <string>

//TODO Need to be re factored
namespace maple
{
	class Scene;
	namespace component
	{
		class MAPLE_EXPORT LuaComponent
		{
		  public:
			friend class MetaFile;
			LuaComponent(const std::string &file, Scene *scene);
			LuaComponent();
			~LuaComponent();
			auto onInit() -> void;
			auto reload() -> void;
			auto loadScript() -> void;
			auto onImGui() -> void;
			auto isLoaded() -> bool;
			auto setFilePath(const std::string &fileName) -> void;

			template <typename Archive>
			auto save(Archive &archive) const -> void
			{
				archive(
				    cereal::make_nvp("entity", entity),
				    cereal::make_nvp("filePath", file));
				metaFile.save(this, file + ".meta");
			}

			template <typename Archive>
			auto load(Archive &archive) -> void
			{
				archive(
				    cereal::make_nvp("entity", entity),
				    cereal::make_nvp("filePath", file));
				init();
			}

			inline auto &getFileName() const
			{
				return file;
			}
			inline auto setScene(Scene *val)
			{
				scene = val;
			}

			auto        saveNewFile(const std::string &fileName) -> void;
			auto        init() -> void;
			std::string file;
			std::string className;

			std::shared_ptr<luabridge::LuaRef> table;
			std::shared_ptr<luabridge::LuaRef> onInitFunc;
			std::shared_ptr<luabridge::LuaRef> onUpdateFunc;
			Scene *                            scene = nullptr;

			MetaFile metaFile;
		};
	}        // namespace component
};           // namespace maple