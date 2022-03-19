//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "MeshLoader.h"
#include "GLTFLoader.h"
#include "OBJLoader.h"
#include "FBXLoader.h"
#include "Others/StringUtils.h"
#include "Others/Console.h"
#include "Application.h"

namespace maple
{
	ModelLoaderFactory::ModelLoaderFactory()
	{
		addModelLoader<GLTFLoader>();
		addModelLoader<OBJLoader>();
		addModelLoader<FBXLoader>();
	}

	auto ModelLoaderFactory::load(const std::string& obj, std::unordered_map<std::string, std::shared_ptr<Mesh>>& meshes, std::shared_ptr<Skeleton>& skeleton) -> void
	{
		auto extension = StringUtils::getExtension(obj);

		if (auto loader = loaders.find(extension); loader != loaders.end()) 
		{
			loader->second->load(obj, extension, meshes, skeleton);
		}
		else 
		{
			LOGE("Unknown file extension {0}",extension);
		}
	}

	auto MeshLoader::load(const std::string& obj, std::unordered_map<std::string, std::shared_ptr<Mesh>>& meshes, std::shared_ptr<Skeleton>& skeleton) -> void
	{
		Application::getModelLoaderFactory()->load(obj, meshes, skeleton);
	}
};            // namespace maple
