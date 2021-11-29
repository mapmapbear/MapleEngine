//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////


#include "Serialization.h"
#include "Engine/Camera.h"
#include "Engine/Mesh.h"
#include "FileSystem/File.h"
#include "Scene/Component/CameraControllerComponent.h"
#include "Scene/Component/Component.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/Scene.h"

#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>

#include <filesystem>
#include <sstream>

#define ALL_COMPONENTS Transform,                 \
	                   NameComponent,             \
	                   ActiveComponent,           \
	                   Hierarchy,                 \
	                   Camera,                    \
	                   Light,                     \
	                   CameraControllerComponent, \
	                   Model,                     \
	                   MeshRenderer,              \
	                   Material,                  \
	                   Environment

namespace Maple
{
	auto Serialization::serialize(Scene *scene) -> void
	{
		auto              outPath = scene->getPath();
		std::stringstream storage;
		{
			// output finishes flushing its contents when it goes out of scope
			cereal::JSONOutputArchive output{storage};
			output(*scene);
			entt::snapshot{
			    scene->getRegistry()}
			    .entities(output)
			    .component<ALL_COMPONENTS>(output);
		}

		File file(outPath, true);
		file.write(storage.str());
	}

	auto Serialization::loadScene(Scene *scene, const std::string &file) -> void
	{
		File               f(file);
		auto               buffer = f.getBuffer();
		std::istringstream istr;
		istr.str((const char *) buffer.get());
		cereal::JSONInputArchive input(istr);
		input(*scene);
		entt::snapshot_loader{scene->getRegistry()}.entities(input).component<ALL_COMPONENTS>(input);
	}

	auto Serialization::loadMaterial(Material *material, const std::string &file) -> void
	{
		File               f(file);
		if (f.getFileSize() != 0)
		{
			auto               buffer = f.getBuffer();
			std::istringstream istr;
			istr.str((const char *) buffer.get());
			cereal::JSONInputArchive input(istr);
			input(*material);
		}
	}

	auto Serialization::serialize(Material *material) -> void
	{
		auto              outPath = material->getPath();
		std::stringstream storage;
		{
			// output finishes flushing its contents when it goes out of scope
			cereal::JSONOutputArchive output{storage};
			output(*material);
		}

		File file(outPath, true);
		file.write(storage.str());
	}
};        // namespace Maple
