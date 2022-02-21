
//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "GeometryRenderer.h"
#include "Engine/Camera.h"
#include "Engine/GBuffer.h"
#include "Engine/Profiler.h"
#include "Engine/Vertex.h"
#include "Math/BoundingBox.h"
#include "RHI/IndexBuffer.h"
#include "RHI/Pipeline.h"
#include "RHI/Shader.h"
#include "RHI/VertexBuffer.h"
#include "RenderGraph.h"
#include "Scene/Component/Transform.h"
#include "Scene/Scene.h"

#include "RendererData.h"
#include "Application.h"

#include <ecs/ecs.h>

namespace maple
{
	struct GeometryLine
	{
		glm::vec3 start;
		glm::vec3 end;
		glm::vec4 color;
	};

	struct GeometryPoint
	{
		glm::vec3 p1;
		glm::vec4 color;
		float     size;
	};

	struct GeometryTriangle
	{
		glm::vec3 p1;
		glm::vec3 p2;
		glm::vec3 p3;
		glm::vec4 color;
	};

	namespace
	{
		constexpr uint32_t MaxPoints = 10000;
		constexpr uint32_t MaxPointVertices = MaxPoints * 4;
		constexpr uint32_t MaxPointIndices = MaxPoints * 6;
		constexpr uint32_t MaxBatchDrawCalls = 100;
		constexpr uint32_t RendererPointSize = sizeof(PointVertex) * 4;
		constexpr uint32_t RendererPointBufferSize = RendererPointSize * MaxPointVertices;

		constexpr uint32_t MaxLines = 10000;
		constexpr uint32_t MaxLineVertices = MaxLines * 2;
		constexpr uint32_t MaxLineIndices = MaxLines * 6;
		constexpr uint32_t MaxLineBatchDrawCalls = 100;
		constexpr uint32_t RendererLineSize = sizeof(LineVertex) * 4;
		constexpr uint32_t RendererLineBufferSize = RendererLineSize * MaxLineVertices;
	}        // namespace

	namespace component
	{
		struct GeometryRenderData
		{
			std::vector<GeometryLine>     lines;
			std::vector<GeometryTriangle> triangles;
			std::vector<GeometryPoint>    points;


			std::shared_ptr<VertexBuffer> lineVertexBuffers;
			std::shared_ptr<VertexBuffer> pointVertexBuffers;

			std::shared_ptr<IndexBuffer> lineIndexBuffer;
			std::shared_ptr<IndexBuffer> pointIndexBuffer;

			std::vector<std::shared_ptr<DescriptorSet>> pointDescriptorSet;
			std::vector<std::shared_ptr<DescriptorSet>> lineDescriptorSet;

			LineVertex* lineBuffer = nullptr;
			PointVertex* pointBuffer = nullptr;

			uint32_t lineIndexCount = 0;
			uint32_t pointIndexCount = 0;
			uint32_t lineBatchDrawCallIndex = 0;
			uint32_t pointBatchDrawCallIndex = 0;

			std::shared_ptr<Shader> lineShader;
			std::shared_ptr<Shader> pointShader;

			GeometryRenderData()
			{
				pointShader = Shader::create("shaders/BatchPoint.shader");
				lineShader = Shader::create("shaders/BatchLine.shader");

				{
					DescriptorInfo descriptorInfo{};
					descriptorInfo.layoutIndex = 0;
					descriptorInfo.shader = pointShader.get();
					pointDescriptorSet.emplace_back(DescriptorSet::create(descriptorInfo));

					pointVertexBuffers = VertexBuffer::create(BufferUsage::Dynamic);
					pointVertexBuffers->resize(RendererPointBufferSize);

					std::vector<uint32_t> indices;
					indices.resize(MaxPointIndices);

					for (int32_t i = 0, offset = 0; i < MaxPointIndices; i += 6, offset += 4)
					{
						indices[i] = offset + 0;
						indices[i + 1] = offset + 1;
						indices[i + 2] = offset + 2;

						indices[i + 3] = offset + 2;
						indices[i + 4] = offset + 3;
						indices[i + 5] = offset + 0;
					}

					pointIndexBuffer = IndexBuffer::create(indices.data(), MaxPointIndices);
				}
				{
					DescriptorInfo descriptorLineInfo{};
					descriptorLineInfo.layoutIndex = 0;
					descriptorLineInfo.shader = pointShader.get();
					lineDescriptorSet.emplace_back(DescriptorSet::create(descriptorLineInfo));

					lineVertexBuffers = VertexBuffer::create(BufferUsage::Dynamic);
					lineVertexBuffers->resize(RendererLineBufferSize);

					std::vector<uint32_t> indices;
					indices.resize(MaxLineIndices);
					for (int32_t i = 0; i < MaxLineIndices; i++)
					{
						indices[i] = i;
					}
					lineIndexBuffer = IndexBuffer::create(indices.data(), MaxLineIndices);
				}
			}
		};
	}

	auto GeometryRenderer::drawLine(const glm::vec3 &start, const glm::vec3 &end, const glm::vec4 &color) -> void
	{
		auto scene = Application::getSceneManager()->getCurrentScene();
		auto& renderData = scene->getGlobalComponent<component::GeometryRenderData>();
		renderData.lines.push_back({start, end, color});
	}

	auto GeometryRenderer::drawFrustum(const Frustum &frustum) -> void
	{
		auto *vertices = frustum.vertices;

		drawLine(vertices[0], vertices[1]);
		drawLine(vertices[1], vertices[2]);
		drawLine(vertices[2], vertices[3]);
		drawLine(vertices[3], vertices[0]);
		drawLine(vertices[4], vertices[5]);
		drawLine(vertices[5], vertices[6]);
		drawLine(vertices[6], vertices[7]);
		drawLine(vertices[7], vertices[4]);
		drawLine(vertices[0], vertices[4]);
		drawLine(vertices[1], vertices[5]);
		drawLine(vertices[2], vertices[6]);
		drawLine(vertices[3], vertices[7]);
	}

	auto GeometryRenderer::drawRect(int32_t x, int32_t y, int32_t width, int32_t height) -> void
	{
		auto right  = x + width;
		auto bottom = y + height;

		drawLine({x, y, 0}, {right, y, 0});
		drawLine({right, y, 0}, {right, bottom, 0});
		drawLine({right, bottom, 0}, {x, bottom, 0});
		drawLine({x, bottom, 0}, {x, y, 0});
	}

	auto GeometryRenderer::drawBox(const BoundingBox &box, const glm::vec4 &color) -> void
	{
		glm::vec3 uuu = box.max;
		glm::vec3 lll = box.min;
		glm::vec3 ull(uuu.x, lll.y, lll.z);
		glm::vec3 uul(uuu.x, uuu.y, lll.z);
		glm::vec3 ulu(uuu.x, lll.y, uuu.z);
		glm::vec3 luu(lll.x, uuu.y, uuu.z);
		glm::vec3 llu(lll.x, lll.y, uuu.z);
		glm::vec3 lul(lll.x, uuu.y, lll.z);

		drawLine(luu, uuu, color);
		drawLine(lul, uul, color);
		drawLine(llu, ulu, color);
		drawLine(lll, ull, color);
		drawLine(lul, lll, color);
		drawLine(uul, ull, color);
		drawLine(luu, llu, color);
		drawLine(uuu, ulu, color);
		drawLine(lll, llu, color);
		drawLine(ull, ulu, color);
		drawLine(lul, luu, color);
		drawLine(uul, uuu, color);
	}

	auto GeometryRenderer::drawTriangle(const glm::vec3 &v0, const glm::vec3 &v1, const glm::vec3 &v2, const glm::vec4 &color) -> void
	{
		auto scene = Application::getSceneManager()->getCurrentScene();
		auto& renderData = scene->getGlobalComponent<component::GeometryRenderData>();
		renderData.triangles.push_back({v0, v1, v2, color});
	}

	namespace on_begin_scene 
	{
		using Entity = ecs::Chain
			::Read<component::RendererData>
			::Write<component::GeometryRenderData>
			::Read<component::CameraView>
			::To<ecs::Entity>;

		inline auto system(Entity entity, ecs::World world) 
		{
			auto [render, geometry, cameraView] = entity;
			geometry.lineDescriptorSet[0]->setUniform("UniformBufferObject", "projView", &cameraView.projView);
			geometry.pointDescriptorSet[0]->setUniform("UniformBufferObject", "projView", &cameraView.projView);
		}
	}

	namespace on_render_lines
	{
		using Entity = ecs::Chain
			::Read<component::RendererData>
			::Write<component::GeometryRenderData>
			::Write<component::CameraView>
			::To<ecs::Entity>;

		inline auto systemLines(Entity entity, ecs::World world) 
		{
			auto [render, geometry, cameraView] = entity;
			auto commandBuffer = render.commandBuffer;

			if (!geometry.lines.empty())
			{
				geometry.lineDescriptorSet[0]->update();

				PipelineInfo pipelineInfo;
				pipelineInfo.shader = geometry.lineShader;
				pipelineInfo.polygonMode = PolygonMode::Fill;
				pipelineInfo.cullMode = CullMode::Back;
				pipelineInfo.transparencyEnabled = false;
				pipelineInfo.clearTargets = false;
				pipelineInfo.drawType = DrawType::Lines;
				pipelineInfo.colorTargets[0] = render.gbuffer->getBuffer(GBufferTextures::SCREEN);

				auto pipeline = Pipeline::get(pipelineInfo);

				pipeline->bind(commandBuffer);
				geometry.lineVertexBuffers->bind(commandBuffer, pipeline.get());
				geometry.lineBuffer = geometry.lineVertexBuffers->getPointer<LineVertex>();

				for (auto& line : geometry.lines)
				{
					geometry.lineBuffer->vertex = line.start;
					geometry.lineBuffer->color = line.color;
					geometry.lineBuffer++;
					geometry.lineBuffer->vertex = line.end;
					geometry.lineBuffer->color = line.color;
					geometry.lineBuffer++;
					geometry.lineIndexCount += 2;
				}

				geometry.lineVertexBuffers->releasePointer();
				geometry.lineVertexBuffers->unbind();

				geometry.lineIndexBuffer->setCount(geometry.lineIndexCount);
				geometry.lineVertexBuffers->bind(commandBuffer, pipeline.get());
				geometry.lineIndexBuffer->bind(commandBuffer);

				Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, geometry.lineDescriptorSet);
				Renderer::drawIndexed(commandBuffer, DrawType::Lines, geometry.lineIndexCount);

				geometry.lineVertexBuffers->unbind();
				geometry.lineIndexBuffer->unbind();

				geometry.lineIndexCount = 0;

				pipeline->end(commandBuffer);

				geometry.lines.clear();
			}
		}

		inline auto systemPoints(Entity entity, ecs::World world)
		{
			auto [render, geometry, cameraView] = entity;
			auto commandBuffer = render.commandBuffer;

			if (!geometry.points.empty())
			{
				geometry.pointDescriptorSet[0]->update();
				PipelineInfo pipelineInfo;
				pipelineInfo.shader = geometry.pointShader;
				pipelineInfo.polygonMode = PolygonMode::Fill;
				pipelineInfo.cullMode = CullMode::Back;
				pipelineInfo.transparencyEnabled = true;
				pipelineInfo.drawType = DrawType::Triangle;
				pipelineInfo.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;
				pipelineInfo.colorTargets[0] = render.gbuffer->getBuffer(GBufferTextures::SCREEN);

				auto pipeline = Pipeline::get(pipelineInfo);

				pipeline->bind(commandBuffer);
				geometry.pointVertexBuffers->bind(commandBuffer, pipeline.get());
				geometry.pointBuffer = geometry.pointVertexBuffers->getPointer<PointVertex>();

				for (auto& pointInfo : geometry.points)
				{
					auto right = pointInfo.size * cameraView.cameraTransform->getRightDirection();
					auto up = pointInfo.size * cameraView.cameraTransform->getUpDirection();

					geometry.pointBuffer->vertex = pointInfo.p1 - right - up;
					geometry.pointBuffer->color = pointInfo.color;
					geometry.pointBuffer->size = { pointInfo.size, 0.0f };
					geometry.pointBuffer->uv = { -1.0f, -1.0f };
					geometry.pointBuffer++;

					geometry.pointBuffer->vertex = pointInfo.p1 + right - up;
					geometry.pointBuffer->color = pointInfo.color;
					geometry.pointBuffer->size = { pointInfo.size, 0.0f };
					geometry.pointBuffer->uv = { 1.0f, -1.0f };
					geometry.pointBuffer++;

					geometry.pointBuffer->vertex = pointInfo.p1 + right + up;
					geometry.pointBuffer->color = pointInfo.color;
					geometry.pointBuffer->size = { pointInfo.size, 0.0f };
					geometry.pointBuffer->uv = { 1.0f, 1.0f };
					geometry.pointBuffer++;

					geometry.pointBuffer->vertex = pointInfo.p1 - right + up;
					geometry.pointBuffer->color = pointInfo.color;
					geometry.pointBuffer->size = { pointInfo.size, 0.0f };
					geometry.pointBuffer->uv = { -1.0f, 1.0f };
					geometry.pointBuffer++;

					geometry.pointIndexCount += 6;
				}

				geometry.pointVertexBuffers->releasePointer();
				geometry.pointIndexBuffer->setCount(geometry.pointIndexCount);
				geometry.pointIndexBuffer->bind(commandBuffer);

				Renderer::bindDescriptorSets(pipeline.get(), commandBuffer, 0, geometry.pointDescriptorSet);
				Renderer::drawIndexed(commandBuffer, DrawType::Triangle, geometry.pointIndexCount);

				geometry.pointVertexBuffers->unbind();
				geometry.pointIndexBuffer->unbind();

				geometry.pointIndexCount = 0;

				pipeline->end(commandBuffer);

				geometry.pointBatchDrawCallIndex++;

				geometry.points.clear();
			}
		}
	}

	namespace geometry_renderer
	{
		auto registerGeometryRenderer(ExecuteQueue& begin, ExecuteQueue& renderer, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->registerGlobalComponent<component::GeometryRenderData>();
			executePoint->registerWithinQueue<on_begin_scene::system>(begin);
			executePoint->registerWithinQueue<on_render_lines::systemLines>(renderer);
			executePoint->registerWithinQueue<on_render_lines::systemPoints>(renderer);
		}
	}

};        // namespace maple
