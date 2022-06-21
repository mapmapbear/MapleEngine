//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Loader.h"
#include "FBXLoader.h"
#include "GLTFLoader.h"
#include "OBJLoader.h"

#include "Application.h"
#include "Others/Console.h"
#include "Others/StringUtils.h"

namespace maple
{
	AssetsLoaderFactory::AssetsLoaderFactory()
	{
		addModelLoader<GLTFLoader>();
		addModelLoader<OBJLoader>();
		addModelLoader<FBXLoader>();
	}

	auto AssetsLoaderFactory::load(const std::string &obj, std::vector<std::shared_ptr<IResource>> &out) -> void
	{
		auto extension = StringUtils::getExtension(obj);
		auto loader    = loaders.find(extension);
		if (loader == loaders.end())
		{
			LOGE("Unknown file extension {0}", extension);
		}
		else
		{
			if (auto iter = cache.find(obj); iter != cache.end())
			{
				out.insert(out.end(), iter->second.begin(), iter->second.end());
			}
			else
			{
				loader->second->load(obj, extension, out);
				cache[obj] = {out.begin(), out.end()};
			}
		}
	}

	auto Loader::load(const std::string &obj, std::vector<std::shared_ptr<IResource>> &out) -> void
	{
		Application::getAssetsLoaderFactory()->load(obj, out);
	}
};        // namespace maple
