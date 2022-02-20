//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "RHI/Definitions.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace maple
{
	class Mesh;
	class Texture;
	class Texture2D;
	class TextureCube;
	class TextureDepth;
	class UniformBuffer;
	class Environment;
	class GBuffer;
	class DescriptorSet;
	class Shader;
	class Scene;
	class FrameBuffer;

	class MAPLE_EXPORT PrefilterRenderer
	{
	  public:
		static constexpr int32_t SkyboxSize = 512;

		PrefilterRenderer();
		~PrefilterRenderer();

		auto init() -> void;
		auto present() -> void;
		auto renderScene() -> void;
		auto beginScene(Environment & env) -> void;

	  private:
		auto updateIrradianceDescriptor() -> void;
		auto updatePrefilterDescriptor() -> void;

		auto generateSkybox() -> void;
		auto generateIrradianceMap() -> void;
		auto generatePrefilterMap() -> void;

		auto createPipeline() -> void;
		auto updateUniform() -> void;
		auto createFrameBuffer() -> void;

		/*std::shared_ptr<Pipeline> cubeMapPipeline;
		std::shared_ptr<Pipeline> prefilterPipeline;
		std::shared_ptr<Pipeline> irradiancePipeline;*/

		std::shared_ptr<Shader> irradianceShader;
		std::shared_ptr<Shader> prefilterShader;
		std::shared_ptr<Shader> cubeMapShader;

		std::shared_ptr<DescriptorSet> irradianceSet;
		std::shared_ptr<DescriptorSet> prefilterSet;

		std::shared_ptr<DescriptorSet> cubeMapSet;

		std::shared_ptr<DescriptorSet> currentSet;

		std::shared_ptr<Texture2D>   skyboxCaptureColor;
		std::shared_ptr<TextureCube> skyboxCube;

		std::shared_ptr<Texture2D> irradianceCaptureColor;
		std::shared_ptr<Texture2D> prefilterCaptureColor;
		std::shared_ptr<Texture2D> equirectangularMap;

		std::shared_ptr<TextureDepth> skyboxDepth;

		std::shared_ptr<Mesh> cube;
		std::shared_ptr<Mesh> cube2;
		Environment *         envComponent = nullptr;
	};
};        // namespace maple
