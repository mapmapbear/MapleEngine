//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Renderer2D.h"
#include "Engine/GBuffer.h"

#include "RHI/CommandBuffer.h"
#include "RHI/DescriptorSet.h"
#include "RHI/FrameBuffer.h"
#include "RHI/IndexBuffer.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderPass.h"
#include "RHI/Shader.h"
#include "RHI/SwapChain.h"
#include "RHI/Texture.h"
#include "RHI/UniformBuffer.h"
#include "RHI/VertexBuffer.h"

#include "2d/Sprite.h"
#include "Engine/Camera.h"
#include "Engine/Profiler.h"
#include "Engine/Vertex.h"
#include "Scene/Component/Transform.h"

#include "Application.h"
#include "FileSystem/File.h"
#include "RendererData.h"
#include "Scene/Scene.h"

#include <ecs/ecs.h>
#include <imgui.h>

namespace maple
{
	struct Config2D
	{
		uint32_t maxQuads          = 10000;
		uint32_t quadsSize         = sizeof(Vertex2D) * 4;
		uint32_t bufferSize        = 10000 * sizeof(Vertex2D) * 4;
		uint32_t indiciesSize      = 10000 * 6;
		uint32_t maxTextures       = 16;
		uint32_t maxBatchDrawCalls = 10;
	} config;

	struct Command2D
	{
		const Quad2D *quad;
		glm::mat4     transform;
	};

	namespace component
	{
		struct Renderer2DData
		{
			std::shared_ptr<Shader>        shader;
			std::shared_ptr<DescriptorSet> descriptorSet;
			std::shared_ptr<DescriptorSet> projViewSet;

			std::vector<std::shared_ptr<Texture>> textures;

			bool enableDepth = true;

			std::vector<std::shared_ptr<VertexBuffer>> vertexBuffers;
			std::shared_ptr<IndexBuffer>               indexBuffer;

			std::vector<Command2D> commands;

			struct UniformBufferObject
			{
				glm::mat4 projView;
			} systemBuffer;

			int32_t   textureCount       = 0;
			int32_t   batchDrawCallIndex = 0;
			int32_t   indexCount         = 0;
			Vertex2D *buffer             = nullptr;

			Renderer2DData()
			{
				textures.resize(16);
				shader        = Shader::create("shaders/Batch2D.shader");
				projViewSet   = DescriptorSet::create({0, shader.get()});
				descriptorSet = DescriptorSet::create({1, shader.get()});
				vertexBuffers.resize(config.maxBatchDrawCalls);

				for (int32_t i = 0; i < config.maxBatchDrawCalls; i++)
				{
					vertexBuffers[i] = VertexBuffer::create(BufferUsage::Dynamic);
					vertexBuffers[i]->resize(config.bufferSize);
				}

				std::vector<uint32_t> indices(config.indiciesSize);
				uint32_t              offset = 0;
				for (uint32_t i = 0; i < config.indiciesSize; i += 6)
				{
					indices[i]     = offset + 0;
					indices[i + 1] = offset + 1;
					indices[i + 2] = offset + 2;

					indices[i + 3] = offset + 2;
					indices[i + 4] = offset + 3;
					indices[i + 5] = offset + 0;
					offset += 4;
				}

				indexBuffer = IndexBuffer::create(indices.data(), config.indiciesSize);
			}
		};
	}        // namespace component

	namespace render2d
	{
		namespace common
		{
			inline auto present(component::Renderer2DData &data, Pipeline *pipeline, CommandBuffer *cmd) -> void
			{
				PROFILE_FUNCTION();
				if (data.indexCount == 0)
				{
					data.vertexBuffers[data.batchDrawCallIndex]->releasePointer();
					return;
				}

				data.descriptorSet->setTexture("textures", data.textures);
				data.descriptorSet->update(cmd);

				pipeline->bind(cmd);

				data.indexBuffer->setCount(data.indexCount);
				data.indexBuffer->bind(cmd);

				data.vertexBuffers[data.batchDrawCallIndex]->releasePointer();
				data.vertexBuffers[data.batchDrawCallIndex]->bind(cmd, pipeline);

				Renderer::bindDescriptorSets(pipeline, cmd, 0, {data.projViewSet, data.descriptorSet});
				Renderer::drawIndexed(cmd, DrawType::Triangle, data.indexCount);

				data.vertexBuffers[data.batchDrawCallIndex]->unbind();
				data.indexBuffer->unbind();
				data.indexCount = 0;

				data.batchDrawCallIndex++;

				pipeline->end(cmd);
			}

			inline auto flush(component::Renderer2DData &data, Pipeline *pipeline, CommandBuffer *cmd) -> void
			{
				PROFILE_FUNCTION();
				present(data, pipeline, cmd);
				data.textureCount = 0;
				data.vertexBuffers[data.batchDrawCallIndex]->unbind();
				data.buffer = data.vertexBuffers[data.batchDrawCallIndex]->getPointer<Vertex2D>();
			}

			inline auto submitTexture(component::Renderer2DData &data, const std::shared_ptr<Texture> &texture, Pipeline *pipeline, CommandBuffer *cmd) -> int32_t
			{
				PROFILE_FUNCTION();

				float result = 0.0f;
				bool  found  = false;

				for (uint32_t i = 0; i < data.textureCount; i++)
				{
					if (data.textures[i] == texture)        //Temp
					{
						return i + 1;
					}
				}

				if (data.textureCount >= config.maxTextures)
				{
					common::flush(data, pipeline, cmd);
				}
				data.textures[data.textureCount] = texture;
				data.textureCount++;
				return data.textureCount;
			}
		}        // namespace common

		namespace on_begin_scene
		{
			using Entity = ecs::Registry ::Modify<component::Renderer2DData>::Fetch<component::CameraView>::Fetch<component::RendererData>::To<ecs::Entity>;

			using SpriteDefine = ecs::Registry ::Modify<component::Sprite>::Modify<component::Transform>;

			using SpriteEntity = SpriteDefine ::To<ecs::Entity>;

			using Group = SpriteDefine ::To<ecs::Group>;

			using AnimatedSpriteDefine = ecs::Registry ::Modify<component::AnimatedSprite>::Modify<component::Transform>;

			using AnimatedSpriteEntity = AnimatedSpriteDefine ::To<ecs::Entity>;

			using AnimatedQuery = AnimatedSpriteDefine ::To<ecs::Group>;

			inline auto system(Entity entity, Group query, AnimatedQuery animatedQuery, ecs::World world)
			{
				auto [data, camera, render] = entity;

				data.systemBuffer.projView = camera.projView;
				data.commands.clear();
				query.forEach([&](SpriteEntity spriteEntity) {
					auto [sprite, trans] = spriteEntity;
					data.commands.push_back({&sprite.getQuad(), trans.getWorldMatrix()});
				});

				animatedQuery.forEach([&](AnimatedSpriteEntity spriteEntity) {
					auto [sprite, trans] = spriteEntity;
					data.commands.push_back({&sprite.getQuad(), trans.getWorldMatrix()});
				});

				std::sort(data.commands.begin(), data.commands.end(), [](Command2D &a, Command2D &b) {
					return a.transform[3][2] < b.transform[3][2];
				});

				if (!data.commands.empty())
				{
					data.projViewSet->setUniformBufferData("UniformBufferObject", &data.systemBuffer);
					data.projViewSet->update(render.commandBuffer);
				}
			}
		}        // namespace on_begin_scene

		namespace on_render
		{
			using Entity = ecs::Registry ::Modify<component::Renderer2DData>::Fetch<component::CameraView>::Fetch<component::RendererData>::To<ecs::Entity>;

			inline auto system(Entity entity, ecs::World world)
			{
				auto [data, camera, render] = entity;

				if (data.commands.empty())
					return;

				PipelineInfo pipeInfo;
				pipeInfo.shader              = data.shader;
				pipeInfo.cullMode            = CullMode::None;
				pipeInfo.transparencyEnabled = true;
				pipeInfo.depthBiasEnabled    = false;
				pipeInfo.clearTargets        = false;
				if (data.enableDepth)
					pipeInfo.depthTarget = render.gbuffer->getDepthBuffer();
				pipeInfo.colorTargets[0] = render.gbuffer->getBuffer(GBufferTextures::SCREEN);
				pipeInfo.blendMode       = BlendMode::SrcAlphaOneMinusSrcAlpha;
				auto pipeline            = Pipeline::get(pipeInfo);

				data.vertexBuffers[data.batchDrawCallIndex]->bind(render.commandBuffer, pipeline.get());
				data.buffer = data.vertexBuffers[data.batchDrawCallIndex]->getPointer<Vertex2D>();

				for (auto &command : data.commands)
				{
					if (data.indexCount >= config.indiciesSize)
					{
						common::flush(data, pipeline.get(), render.commandBuffer);
					}
					auto &quad2d    = command.quad;
					auto &transform = command.transform;

					glm::vec4 min = transform * glm::vec4(quad2d->getOffset(), 0, 1.f);
					glm::vec4 max = transform * glm::vec4(quad2d->getOffset() + glm::vec2{quad2d->getWidth(), quad2d->getHeight()}, 0, 1.f);

					const auto &color   = quad2d->getColor();
					const auto &uv      = quad2d->getTexCoords();
					const auto  texture = quad2d->getTexture();

					int32_t textureSlot = -1;
					if (texture)
						textureSlot = common::submitTexture(data, texture, pipeline.get(), render.commandBuffer);

					data.buffer->vertex = glm::vec3(min.x, min.y, 0.0f);
					data.buffer->color  = color;
					data.buffer->uv     = glm::vec3(uv[0], textureSlot);
					data.buffer++;

					data.buffer->vertex = glm::vec3(max.x, min.y, 0.0f);
					data.buffer->color  = color;
					data.buffer->uv     = glm::vec3(uv[1], textureSlot);
					data.buffer++;

					data.buffer->vertex = glm::vec3(max.x, max.y, 0.0f);
					data.buffer->color  = color;
					data.buffer->uv     = glm::vec3(uv[2], textureSlot);
					data.buffer++;

					data.buffer->vertex = glm::vec3(min.x, max.y, 0.0f);
					data.buffer->color  = color;
					data.buffer->uv     = glm::vec3(uv[3], textureSlot);
					data.buffer++;
					data.indexCount += 6;
				}

				common::present(data, pipeline.get(), render.commandBuffer);

				data.batchDrawCallIndex = 0;
			}
		}        // namespace on_render

		auto registerRenderer2D(ExecuteQueue &begin, ExecuteQueue &renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::Renderer2DData>();
			executePoint->registerWithinQueue<on_begin_scene::system>(begin);
			executePoint->registerWithinQueue<on_render::system>(renderer);
		}
	}        // namespace render2d

};        // namespace maple
