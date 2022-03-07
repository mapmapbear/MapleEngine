//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "MeshLoader.h"
#include "GLTFLoader.h"
#include "OBJLoader.h"
#include "Others/StringUtils.h"
#include "Others/Console.h"
#include "Application.h"

namespace maple
{
	ModelLoaderFactory::ModelLoaderFactory()
	{
		addModelLoader<GLTFLoader>();
		addModelLoader<OBJLoader>();
	}

	auto ModelLoaderFactory::load(const std::string& obj, std::unordered_map<std::string, std::shared_ptr<Mesh>>& meshes) -> void
	{
		auto extension = StringUtils::getExtension(obj);

		if (auto loader = loaders.find(extension); loader != loaders.end()) 
		{
			loader->second->load(obj, meshes);
		}
		else 
		{
			LOGE("Unknown file extension {0}",extension);
		}
	}

	auto MeshLoader::load(const std::string& obj, std::unordered_map<std::string, std::shared_ptr<Mesh>>& meshes) -> void
	{
		Application::getModelLoaderFactory()->load(obj, meshes);
	}
};            // namespace maple
