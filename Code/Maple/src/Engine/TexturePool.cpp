//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "TexturePool.h"

namespace maple
{
	auto TexturePool::addSprite(const std::string &file) -> Quad2D *
	{
		if (auto iter = mapping.find(file); iter != mapping.end())
		{
			return iter->second->addSprite(file);
		}
		if (atlas.empty())
		{
			createTextureAtlas();
		}
		auto quad = atlas.back().addSprite(file);
		if (quad == nullptr)
		{
			auto txt = createTextureAtlas();
			quad     = txt->addSprite(file);
		}
		mapping[file] = &atlas.back();
		return quad;
	}

	auto TexturePool::addSprite(const std::string &uniqueName, const std::vector<uint8_t> &data, uint32_t w, uint32_t h, bool flipY) -> Quad2D *
	{
		if (auto iter = mapping.find(uniqueName); iter != mapping.end())
		{
			return iter->second->addSprite(uniqueName, flipY);
		}

		if (atlas.empty())
		{
			createTextureAtlas();
		}
		auto quad = atlas.back().addSprite(uniqueName, data, w, h, flipY);
		if (quad == nullptr)
		{        //full.. create a new one
			auto txt = createTextureAtlas();
			quad     = txt->addSprite(uniqueName, flipY);
		}
		mapping[uniqueName] = &atlas.back();
		return quad;
	}

	auto TexturePool::createTextureAtlas() -> TextureAtlas *
	{
		return &atlas.emplace_back();
	}
};        // namespace maple
