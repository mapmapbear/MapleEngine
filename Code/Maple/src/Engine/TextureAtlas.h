//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "Quad2D.h"
#include "QuadTree.h"
#include "RHI/Texture.h"

namespace maple
{
	class Texture;
	class MAPLE_EXPORT TextureAtlas
	{
	  public:
		TextureAtlas(uint32_t w = 4096, uint32_t h = 4096);

		auto addSprite(const std::string &file, bool flipY = false) -> Quad2D *;
		auto addSprite(const std::string &uniqueName, const std::vector<uint8_t> &, uint32_t w, uint32_t h, bool flipY) -> Quad2D *;

		inline auto getTexture()
		{
			return texture;
		}
		inline auto getUsage()
		{
			return usage;
		}

	  private:
		auto update(const std::string &uniqueName, const uint8_t *buffer, uint32_t w, uint32_t h, bool flipY) -> Quad2D *;

		size_t                                  rlid   = 1;
		size_t                                  wasted = 0;
		float                                   usage  = 0.f;
		std::shared_ptr<Texture2D>              texture;
		std::unordered_map<std::string, Quad2D> offsets;
		QuadTree<size_t, LeftOver>              leftovers;

		static const int16_t MINLOSIZE = 4;

		std::pair<int16_t, int16_t> border;
		std::pair<int16_t, int16_t> yRange;
	};
};        // namespace maple
