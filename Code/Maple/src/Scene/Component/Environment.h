//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include <entt/entt.hpp>
#include <glm/vec3.hpp>
#include <string>

namespace maple
{
	class TextureCube;
	class Texture;
	class Texture2D;

	namespace component
	{
		struct Environment
		{
			static constexpr int32_t IrradianceMapSize = 32;
			static constexpr int32_t PrefilterMapSize = 128;

			std::shared_ptr<Texture2D>   equirectangularMap;
			std::shared_ptr<TextureCube> environment;
			std::shared_ptr<TextureCube> prefilteredEnvironment;
			std::shared_ptr<TextureCube> irradianceMap;

			bool pseudoSky = false;

			uint32_t    numMips = 0;
			uint32_t    width = 0;
			uint32_t    height = 0;

			std::string filePath;

			glm::vec3 skyColorTop = glm::vec3(0.5, 0.7, 0.8) * 1.05f;
			glm::vec3 skyColorBottom = glm::vec3(0.9, 0.9, 0.95);
		};
	}
}
