//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "EnvironmentModule.h"
#include "Scene/Component/Environment.h"
#include "RHI/Texture.h"

namespace maple
{
	auto environment::init(component::Environment& env, const std::string& fileName) -> void
	{
		env.filePath = fileName;
		if (fileName != "")
		{
			TextureLoadOptions options(false, true, true);
			TextureParameters  parameters(TextureFormat::RGBA32, TextureWrap::ClampToEdge);
			env.equirectangularMap = Texture2D::create(fileName, fileName, parameters, options);
			env.width = env.equirectangularMap->getWidth();
			env.height = env.equirectangularMap->getHeight();
			env.numMips = env.equirectangularMap->getMipMapLevels();
			env.irradianceMap = TextureCube::create(component::Environment::IrradianceMapSize, TextureFormat::RGBA32, 0);
			env.prefilteredEnvironment = TextureCube::create(component::Environment::PrefilterMapSize, TextureFormat::RGBA32, 7);
		}
	}
};

