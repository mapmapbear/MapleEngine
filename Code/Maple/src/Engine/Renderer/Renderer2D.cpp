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

#include "Engine/Camera.h"
#include "Engine/Profiler.h"
#include "Scene/Component/Sprite.h"

#include "Application.h"
#include "FileSystem/File.h"
#include "Scene/Scene.h"

#include <imgui.h>

namespace maple
{
	namespace
	{
		auto getCommandBuffer() -> CommandBuffer *
		{
			return Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();
		}
	}        // namespace

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

	struct Renderer2D::Renderer2DData
	{
		std::shared_ptr<Shader>        shader;
		std::shared_ptr<Pipeline>      pipeline;
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
	};

	Renderer2D::Renderer2D(bool enableDepth)
	{
		data              = new Renderer2DData();
		data->enableDepth = enableDepth;
		data->textures.resize(16);
	}

	Renderer2D::~Renderer2D()
	{
		delete data;
	}

	auto Renderer2D::init(const std::shared_ptr<GBuffer> &buffer) -> void
	{
		gbuffer             = buffer;
		data->shader        = Shader::create("shaders/Batch2D.shader");
		data->projViewSet   = DescriptorSet::create({0, data->shader.get()});
		data->descriptorSet = DescriptorSet::create({1, data->shader.get()});
		data->vertexBuffers.resize(config.maxBatchDrawCalls);

		for (int32_t i = 0; i < config.maxBatchDrawCalls; i++)
		{
			data->vertexBuffers[i] = VertexBuffer::create(BufferUsage::Dynamic);
			data->vertexBuffers[i]->resize(config.bufferSize);
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

		data->indexBuffer = IndexBuffer::create(indices.data(), config.indiciesSize);
	}

	auto Renderer2D::renderScene() -> void
	{
		PROFILE_FUNCTION();

		if (data->pipeline == nullptr && renderTexture)
		{
			PipelineInfo pipeInfo;
			pipeInfo.shader              = data->shader;
			pipeInfo.cullMode            = CullMode::None;
			pipeInfo.transparencyEnabled = true;
			pipeInfo.depthBiasEnabled    = false;
			pipeInfo.clearTargets        = false;
			if (data->enableDepth)
				pipeInfo.depthTarget = gbuffer->getDepthBuffer();
			pipeInfo.colorTargets[0] = renderTexture;
			pipeInfo.blendMode       = BlendMode::SrcAlphaOneMinusSrcAlpha;
			data->pipeline           = Pipeline::get(pipeInfo);
		}

		data->vertexBuffers[data->batchDrawCallIndex]->bind(getCommandBuffer(), data->pipeline.get());
		data->buffer = data->vertexBuffers[data->batchDrawCallIndex]->getPointer<Vertex2D>();

		for (auto &command : data->commands)
		{
			if (data->indexCount >= config.indiciesSize)
			{
				flush();
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
				textureSlot = submitTexture(texture);

			data->buffer->vertex = glm::vec3(min.x, min.y, 0.0f);
			data->buffer->color  = color;
			data->buffer->uv     = glm::vec3(uv[0], textureSlot);
			data->buffer++;

			data->buffer->vertex = glm::vec3(max.x, min.y, 0.0f);
			data->buffer->color  = color;
			data->buffer->uv     = glm::vec3(uv[1], textureSlot);
			data->buffer++;

			data->buffer->vertex = glm::vec3(max.x, max.y, 0.0f);
			data->buffer->color  = color;
			data->buffer->uv     = glm::vec3(uv[2], textureSlot);
			data->buffer++;

			data->buffer->vertex = glm::vec3(min.x, max.y, 0.0f);
			data->buffer->color  = color;
			data->buffer->uv     = glm::vec3(uv[3], textureSlot);
			data->buffer++;
			data->indexCount += 6;
		}

		present();

		data->batchDrawCallIndex = 0;
	}

	auto Renderer2D::beginScene(Scene *scene, const glm::mat4 &projView) -> void
	{
		PROFILE_FUNCTION();

		auto camera = scene->getCamera();
		if (camera.first == nullptr)
		{
			return;
		}

		data->systemBuffer.projView = projView;
		data->commands.clear();

		data->projViewSet->setUniformBufferData("UniformBufferObject", &data->systemBuffer);
		data->projViewSet->update();

		auto &registry = scene->getRegistry();
		auto  group    = registry.group<Sprite>(entt::get<Transform>);

		{
			PROFILE_SCOPE("Submit Sprites");
			for (auto entity : group)
			{
				const auto &[sprite, trans] = group.get<Sprite, Transform>(entity);
				data->commands.push_back({&sprite.getQuad(), trans.getWorldMatrix()});
			}
		}
		{
			PROFILE_SCOPE("Submit AnimatedSprite");
			auto group2 = registry.group<AnimatedSprite>(entt::get<Transform>);
			for (auto entity : group2)
			{
				const auto &[anim, trans] = group2.get<AnimatedSprite, Transform>(entity);
				data->commands.push_back({&anim.getQuad(), trans.getWorldMatrix()});
			}
		}

		std::sort(data->commands.begin(), data->commands.end(), [](Command2D &a, Command2D &b) {
			return a.transform[3][2] < b.transform[3][2];
		});
	}

	auto Renderer2D::setRenderTarget(std::shared_ptr<Texture> texture, bool rebuildFramebuffer) -> void
	{
		PROFILE_FUNCTION();
		Renderer::setRenderTarget(texture, rebuildFramebuffer);
		data->pipeline = nullptr;
	}

	auto Renderer2D::submitTexture(const std::shared_ptr<Texture> &texture) -> int32_t
	{
		PROFILE_FUNCTION();

		float result = 0.0f;
		bool  found  = false;

		for (uint32_t i = 0; i < data->textureCount; i++)
		{
			if (data->textures[i] == texture)        //Temp
			{
				return i + 1;
			}
		}

		if (data->textureCount >= config.maxTextures)
		{
			flush();
		}
		data->textures[data->textureCount] = texture;
		data->textureCount++;
		return data->textureCount;
	}

	auto Renderer2D::flush() -> void
	{
		PROFILE_FUNCTION();
		present();
		data->textureCount = 0;
		data->vertexBuffers[data->batchDrawCallIndex]->unbind();
		data->buffer = data->vertexBuffers[data->batchDrawCallIndex]->getPointer<Vertex2D>();
	}

	auto Renderer2D::present() -> void
	{
		PROFILE_FUNCTION();
		if (data->indexCount == 0)
		{
			data->vertexBuffers[data->batchDrawCallIndex]->releasePointer();
			return;
		}

		data->descriptorSet->setTexture("textures", data->textures);
		data->descriptorSet->update();

		auto cmd = getCommandBuffer();

		if (data->pipeline)
		{
			data->pipeline->bind(cmd);

			data->indexBuffer->setCount(data->indexCount);
			data->indexBuffer->bind(cmd);

			data->vertexBuffers[data->batchDrawCallIndex]->releasePointer();
			data->vertexBuffers[data->batchDrawCallIndex]->bind(cmd, data->pipeline.get());

			Renderer::bindDescriptorSets(data->pipeline.get(), cmd, 0, {data->projViewSet, data->descriptorSet});
			Renderer::drawIndexed(cmd, DrawType::Triangle, data->indexCount);

			data->vertexBuffers[data->batchDrawCallIndex]->unbind();
			data->indexBuffer->unbind();
			data->indexCount = 0;

			data->batchDrawCallIndex++;

			data->pipeline->end(cmd);
		}
	}
};        // namespace maple
