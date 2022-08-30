//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "DDGIRenderer.h"
#include "DDGIVisualization.h"

#include "Engine/CaptureGraph.h"
#include "Engine/GBuffer.h"
#include "Engine/Material.h"
#include "Engine/Mesh.h"
#include "Engine/PathTracer/TracedData.h"
#include "Engine/Raytrace/AccelerationStructure.h"
#include "Engine/Raytrace/RaytraceScale.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/Renderer/SkyboxRenderer.h"

#include "RHI/BatchTask.h"
#include "RHI/DescriptorPool.h"
#include "RHI/DescriptorSet.h"
#include "RHI/GraphicsContext.h"
#include "RHI/IndexBuffer.h"
#include "RHI/Pipeline.h"
#include "RHI/RenderDevice.h"
#include "RHI/StorageBuffer.h"
#include "RHI/Texture.h"
#include "RHI/VertexBuffer.h"

#include "Math/MathUtils.h"
#include "Others/Randomizer.h"

#include "Scene/Component/Bindless.h"
#include "Scene/Component/BoundingBox.h"
#include "Scene/Component/Light.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Transform.h"
#include "Scene/System/BindlessModule.h"

#include <scope_guard.hpp>

namespace maple
{
	namespace
	{
		constexpr uint32_t MAX_SCENE_MESH_INSTANCE_COUNT    = 1024;
		constexpr uint32_t MAX_SCENE_LIGHT_COUNT            = 300;
		constexpr uint32_t MAX_SCENE_MATERIAL_COUNT         = 4096;
		constexpr uint32_t MAX_SCENE_MATERIAL_TEXTURE_COUNT = MAX_SCENE_MATERIAL_COUNT * 4;
	}        // namespace

	namespace ddgi
	{
		namespace component
		{
			struct DDGIPipelineInternal
			{
				//raytrace pass.
				Texture2D::Ptr radiance;
				Texture2D::Ptr directionDepth;

				Texture2D::Ptr irradiance[2];
				Texture2D::Ptr depth[2];

				//result
				Texture2D::Ptr sampleProbeGrid;
				int32_t        frames   = 0;
				int32_t        pingPong = 0;
				Randomizer     rand;
			};

			struct RaytracePass
			{
				Shader::Ptr   shader;
				Pipeline::Ptr pipeline;

				/*StorageBuffer::Ptr  lightBuffer;
				StorageBuffer::Ptr  materialBuffer;
				StorageBuffer::Ptr  transformBuffer;
				DescriptorPool::Ptr descriptorPool
				Material::Ptr             defaultMaterial;
				std::vector<Texture::Ptr> shaderTextures;
				DescriptorSet::Ptr sceneDescriptor;
				DescriptorSet::Ptr vboDescriptor;
				DescriptorSet::Ptr iboDescriptor;
				DescriptorSet::Ptr materialDescriptor;
				DescriptorSet::Ptr textureDescriptor;*/

				DescriptorSet::Ptr samplerDescriptor;
				DescriptorSet::Ptr outpuDescriptor;

				struct PushConstants
				{
					glm::mat4 randomOrientation;
					uint32_t  numFrames;
					uint32_t  infiniteBounces = 1;
					int32_t   numLights;
					float     intensity;
				} pushConsts;
				bool firstFrame = true;
			};

			struct ProbeUpdatePass
			{
				Shader::Ptr irradanceShader;
				Shader::Ptr depthShader;

				Pipeline::Ptr irradianceProbePipeline;
				Pipeline::Ptr depthProbePipeline;

				struct PushConsts
				{
					uint32_t firstFrame = 0;
				} pushConsts;

				std::vector<DescriptorSet::Ptr> irradianceDescriptors;
				std::vector<DescriptorSet::Ptr> depthDescriptors;
			};

			struct BorderUpdatePass
			{
				Shader::Ptr irradanceShader;
				Shader::Ptr depthShader;

				Pipeline::Ptr irradianceProbePipeline;
				Pipeline::Ptr depthProbePipeline;

				std::vector<DescriptorSet::Ptr> irradianceDescriptors;
				std::vector<DescriptorSet::Ptr> depthDescriptors;
			};

			struct SampleProbePass
			{
				Shader::Ptr shader;

				Pipeline::Ptr pipeline;

				std::vector<DescriptorSet::Ptr> descriptors;
			};
		}        // namespace component

		namespace init
		{
			inline auto initializeProbeGrid(
			    ddgi::component::DDGIPipeline &        pipeline,
			    ddgi::component::DDGIPipelineInternal &internal,
			    component::DDGIUniform &               uniform,
			    ecs::World                             world)
			{
				uint32_t totalProbes = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;

				{
					internal.radiance = Texture2D::create();
					internal.radiance->setName("DDGI Raytrace Radiance");
					internal.radiance->buildTexture(TextureFormat::RGBA16, pipeline.raysPerProbe, totalProbes);

					internal.directionDepth = Texture2D::create();
					internal.directionDepth->setName("DDGI Raytrace Direction Depth");
					internal.directionDepth->buildTexture(TextureFormat::RGBA16, pipeline.raysPerProbe, totalProbes);
				}

				{
					// 1-pixel of padding surrounding each probe, 1-pixel padding surrounding entire texture for alignment.
					const int32_t irradianceWidth  = (IrradianceOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
					const int32_t irradianceHeight = (IrradianceOctSize + 2) * uniform.probeCounts.z + 2;
					const int32_t depthWidth       = (DepthOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
					const int32_t depthHeight      = (DepthOctSize + 2) * uniform.probeCounts.z + 2;

					uniform.irradianceTextureWidth  = irradianceWidth;
					uniform.irradianceTextureHeight = irradianceHeight;

					uniform.depthTextureWidth  = depthWidth;
					uniform.depthTextureHeight = depthHeight;

					for (int32_t i = 0; i < 2; i++)
					{
						internal.irradiance[i] = Texture2D::create();
						internal.depth[i]      = Texture2D::create();

						internal.depth[i]->buildTexture(TextureFormat::RG16F, depthWidth, depthHeight);
						internal.depth[i]->setName("DDGI Depth Probe Grid " + std::to_string(i));

						internal.irradiance[i]->buildTexture(TextureFormat::RGBA16, irradianceWidth, irradianceHeight);
						internal.irradiance[i]->setName("DDGI Irradiance Probe Grid " + std::to_string(i));
					}
				}

				internal.sampleProbeGrid = Texture2D::create();
				internal.sampleProbeGrid->setName("DDGI Sample Probe Grid");
				internal.sampleProbeGrid->buildTexture(TextureFormat::RGBA16, pipeline.width, pipeline.height);
			}
		}        // namespace init
	}            // namespace ddgi

	namespace trace_rays
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Modify<ddgi::component::RaytracePass>
			::Modify<ddgi::component::DDGIPipelineInternal>
			::Modify<ddgi::component::DDGIPipeline>
			::Fetch<ddgi::component::DDGIUniform>
			::To<ecs::Entity>;
		// clang-format on

		inline auto system(Entity                                   entity,
		                   component::RendererData &                renderData,
		                   const global::component::RaytracingDescriptor &descriptor,
		                   ecs::World                               world)
		{
			auto [raytracePass, internal, pipeline, uniform] = entity;
			pipeline.currentIrrdance                         = internal.irradiance[1 - internal.pingPong];
			pipeline.currentDepth                            = internal.depth[1 - internal.pingPong];

			raytracePass.samplerDescriptor->setTexture("uIrradiance", internal.irradiance[1 - internal.pingPong]);
			raytracePass.samplerDescriptor->setTexture("uDepth", internal.depth[1 - internal.pingPong]);
			raytracePass.outpuDescriptor->setTexture("iRadiance", internal.radiance);
			raytracePass.outpuDescriptor->setTexture("iDirectionDistance", internal.directionDepth);

			/*	raytracePass.sceneDescriptor->update(renderData.commandBuffer);
			raytracePass.vboDescriptor->update(renderData.commandBuffer);
			raytracePass.iboDescriptor->update(renderData.commandBuffer);
			raytracePass.materialDescriptor->update(renderData.commandBuffer);
			raytracePass.textureDescriptor->update(renderData.commandBuffer);*/

			raytracePass.samplerDescriptor->update(renderData.commandBuffer);
			raytracePass.outpuDescriptor->update(renderData.commandBuffer);

			raytracePass.pipeline->bind(renderData.commandBuffer);
			raytracePass.pushConsts.numLights       = descriptor.numLights;
			raytracePass.pushConsts.infiniteBounces = pipeline.infiniteBounce && internal.frames != 0 ? 1 : 0;
			raytracePass.pushConsts.intensity       = pipeline.intensity;
			raytracePass.pushConsts.numFrames       = internal.frames;

			auto vec3                                 = glm::normalize(glm::vec3(internal.rand.nextReal(-1.f, 1.f),
                                                 internal.rand.nextReal(-1.f, 1.f),
                                                 internal.rand.nextReal(-1.f, 1.f)));
			raytracePass.pushConsts.randomOrientation = glm::mat4_cast(
			    glm::angleAxis(internal.rand.nextReal(0.f, 1.f) * float(M_PI) * 2.0f, vec3));

			for (auto &push : raytracePass.pipeline->getShader()->getPushConstants())
			{
				push.setData(&raytracePass.pushConsts);
			}

			raytracePass.pipeline->getShader()->bindPushConstants(renderData.commandBuffer, raytracePass.pipeline.get());
			Renderer::bindDescriptorSets(raytracePass.pipeline.get(), renderData.commandBuffer, 0, 
				{
				descriptor.sceneDescriptor, 
				descriptor.vboDescriptor, 
				descriptor.iboDescriptor, 
				descriptor.materialDescriptor, 
				descriptor.textureDescriptor, 
				raytracePass.samplerDescriptor,
				raytracePass.outpuDescriptor});

			uint32_t probleCounts = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;
			raytracePass.pipeline->traceRays(renderData.commandBuffer, uniform.raysPerProbe, probleCounts, 1);
			raytracePass.pipeline->end(renderData.commandBuffer);
		}
	}        // namespace trace_rays

	namespace end_frame
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Modify<ddgi::component::DDGIPipelineInternal>
			::To<ecs::Entity>;
		// clang-format on

		inline auto system(Entity entity, ecs::World world)
		{
			auto [internal]   = entity;
			internal.pingPong = 1 - internal.pingPong;
			internal.frames++;
		}
	}        // namespace end_frame

	namespace probe_update
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Modify<ddgi::component::ProbeUpdatePass>
			::Modify<ddgi::component::RaytracePass>
			::Modify<ddgi::component::DDGIPipelineInternal>
			::Modify<ddgi::component::DDGIPipeline>
			::Fetch<ddgi::component::DDGIUniform>
			::To<ecs::Entity>;
		// clang-format on

		inline auto probeUpdate(
		    ddgi::component::ProbeUpdatePass &  pass,
		    const ddgi::component::DDGIUniform &uniform,
		    const component::RendererData &     renderData,
		    Pipeline::Ptr                       pipeline,
		    std::vector<DescriptorSet::Ptr> &   descriptors)
		{
			for (auto &descriptor : descriptors)
			{
				descriptor->update(renderData.commandBuffer);
			}

			for (auto &push : pipeline->getShader()->getPushConstants())
			{
				pass.pushConsts.firstFrame = renderData.numFrames == 0 ? 1 : 0;
				push.setData(&pass.pushConsts);
			}
			pipeline->bind(renderData.commandBuffer);
			pipeline->getShader()->bindPushConstants(renderData.commandBuffer, pipeline.get());

			const uint32_t dispatchX = static_cast<uint32_t>(uniform.probeCounts.x * uniform.probeCounts.y);
			const uint32_t dispatchY = static_cast<uint32_t>(uniform.probeCounts.z);

			Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, descriptors);

			Renderer::dispatch(renderData.commandBuffer, dispatchX, dispatchY, 1);
			pipeline->end(renderData.commandBuffer);
		}

		inline auto system(Entity entity, component::RendererData &renderData, ecs::World world)
		{
			auto [probeUpdatePass, raytracePass, internal, pipeline, uniform] = entity;
			auto writeIdx                                                     = 1 - internal.pingPong;

			probeUpdatePass.irradianceDescriptors[0]->setTexture("uOutIrradiance", internal.irradiance[writeIdx]);
			probeUpdatePass.irradianceDescriptors[0]->setTexture("uOutDepth", internal.depth[writeIdx]);
			probeUpdatePass.irradianceDescriptors[1]->setTexture("uInputIrradiance", internal.irradiance[internal.pingPong]);
			probeUpdatePass.irradianceDescriptors[1]->setTexture("uInputDepth", internal.depth[internal.pingPong]);
			probeUpdatePass.irradianceDescriptors[1]->setUniform("DDGIUBO", "ddgi", &uniform);
			probeUpdatePass.irradianceDescriptors[2]->setTexture("uInputRadiance", internal.radiance);
			probeUpdatePass.irradianceDescriptors[2]->setTexture("uInputDirectionDepth", internal.directionDepth);

			probeUpdatePass.depthDescriptors[0]->setTexture("uOutIrradiance", internal.irradiance[writeIdx]);
			probeUpdatePass.depthDescriptors[0]->setTexture("uOutDepth", internal.depth[writeIdx]);
			probeUpdatePass.depthDescriptors[1]->setTexture("uInputIrradiance", internal.irradiance[internal.pingPong]);
			probeUpdatePass.depthDescriptors[1]->setTexture("uInputDepth", internal.depth[internal.pingPong]);
			probeUpdatePass.depthDescriptors[1]->setUniform("DDGIUBO", "ddgi", &uniform);
			probeUpdatePass.depthDescriptors[2]->setTexture("uInputRadiance", internal.radiance);
			probeUpdatePass.depthDescriptors[2]->setTexture("uInputDirectionDepth", internal.directionDepth);
			probeUpdate(probeUpdatePass, uniform, renderData, probeUpdatePass.irradianceProbePipeline, probeUpdatePass.irradianceDescriptors);
			probeUpdate(probeUpdatePass, uniform, renderData, probeUpdatePass.depthProbePipeline, probeUpdatePass.depthDescriptors);
		}
	}        // namespace probe_update

	namespace border_update
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Modify<ddgi::component::BorderUpdatePass>
			::Modify<ddgi::component::RaytracePass>
			::Modify<ddgi::component::DDGIPipelineInternal>
			::Modify<ddgi::component::DDGIPipeline>
			::Fetch<ddgi::component::DDGIUniform>
			::To<ecs::Entity>;
		// clang-format on

		inline auto borderUpdate(
		    ddgi::component::BorderUpdatePass & pass,
		    const ddgi::component::DDGIUniform &uniform,
		    const component::RendererData &     renderData,
		    Pipeline::Ptr                       pipeline,
		    std::vector<DescriptorSet::Ptr> &   descriptors)
		{
			for (auto &descriptor : descriptors)
			{
				descriptor->update(renderData.commandBuffer);
			}

			pipeline->bind(renderData.commandBuffer);

			const uint32_t dispatchX = static_cast<uint32_t>(uniform.probeCounts.x * uniform.probeCounts.y);
			const uint32_t dispatchY = static_cast<uint32_t>(uniform.probeCounts.z);

			Renderer::bindDescriptorSets(pipeline.get(), renderData.commandBuffer, 0, descriptors);

			Renderer::dispatch(renderData.commandBuffer, dispatchX, dispatchY, 1);
			pipeline->end(renderData.commandBuffer);
		}

		inline auto system(Entity entity, component::RendererData &renderData, ecs::World world)
		{
			auto [borderUpdatePass, raytracePass, internal, pipeline, uniform] = entity;
			auto writeIdx                                                      = 1 - internal.pingPong;

			borderUpdatePass.irradianceDescriptors[0]->setTexture("iOutputIrradiance", internal.irradiance[writeIdx]);
			borderUpdatePass.irradianceDescriptors[0]->setTexture("iOutputDepth", internal.depth[writeIdx]);

			borderUpdatePass.depthDescriptors[0]->setTexture("iOutputIrradiance", internal.irradiance[writeIdx]);
			borderUpdatePass.depthDescriptors[0]->setTexture("iOutputDepth", internal.depth[writeIdx]);

			borderUpdate(borderUpdatePass, uniform, renderData, borderUpdatePass.irradianceProbePipeline, borderUpdatePass.irradianceDescriptors);
			borderUpdate(borderUpdatePass, uniform, renderData, borderUpdatePass.depthProbePipeline, borderUpdatePass.depthDescriptors);
		}
	}        // namespace border_update

	namespace sample_probe
	{
		// clang-format off
		using Entity = ecs::Registry 
			::Modify<ddgi::component::SampleProbePass>
			::Modify<ddgi::component::DDGIPipelineInternal>
			::Fetch<ddgi::component::DDGIUniform>
			::To<ecs::Entity>;
		// clang-format on

		inline auto system(Entity entity, component::RendererData &renderData, component::CameraView &cameraView, ecs::World world)
		{
			auto [pass, internal, uniform] = entity;
			auto writeIdx                  = 1 - internal.pingPong;

			pass.descriptors[0]->setTexture("outColor", renderData.gbuffer->getBuffer(GBufferTextures::INDIRECT_LIGHTING));
			pass.descriptors[0]->setTexture("uPositionSampler", renderData.gbuffer->getBuffer(GBufferTextures::POSITION));
			pass.descriptors[0]->setTexture("uNormalSampler", renderData.gbuffer->getBuffer(GBufferTextures::NORMALS));
			pass.descriptors[0]->setTexture("uIrradiance", internal.irradiance[writeIdx]);
			pass.descriptors[0]->setTexture("uDepth", internal.depth[writeIdx]);
			pass.descriptors[0]->setUniform("DDGIUBO", "ddgi", &uniform);
			auto pos = glm::vec4(cameraView.cameraTransform->getWorldPosition(), 1.f);
			pass.descriptors[0]->setUniform("UniformBufferObject", "cameraPosition", &pos);
			pass.descriptors[0]->update(renderData.commandBuffer);
			pass.pipeline->bind(renderData.commandBuffer);
			const uint32_t dispatchX = static_cast<uint32_t>(std::ceil(float(renderData.gbuffer->getBuffer(GBufferTextures::INDIRECT_LIGHTING)->getWidth()) / 32.f));
			const uint32_t dispatchY = static_cast<uint32_t>(std::ceil(float(renderData.gbuffer->getBuffer(GBufferTextures::INDIRECT_LIGHTING)->getHeight()) / 32.f));
			Renderer::bindDescriptorSets(pass.pipeline.get(), renderData.commandBuffer, 0, pass.descriptors);
			Renderer::dispatch(renderData.commandBuffer, dispatchX, dispatchY, 1);
			pass.pipeline->end(renderData.commandBuffer);
		}
	}        // namespace sample_probe

	namespace ddgi
	{
		namespace delegates
		{
			inline auto uniformChanged(ddgi::component::DDGIPipeline &pipeline, Entity entity, ecs::World world)
			{
				auto &uniform      = entity.getComponent<component::DDGIUniform>();
				auto &bBox         = entity.getComponent<maple::component::BoundingBoxComponent>();
				auto &raytracePass = entity.getComponent<ddgi::component::RaytracePass>();

				glm::vec3 sceneLength = bBox.box->max - bBox.box->min;
				// Add 2 more probes to fully cover scene.
				uniform.probeCounts  = glm::ivec4(glm::ivec3(sceneLength / pipeline.probeDistance) + glm::ivec3(2), 1);
				uint32_t totalProbes = uniform.probeCounts.x * uniform.probeCounts.y * uniform.probeCounts.z;

				uniform.startPosition = glm::vec4(bBox.box->min, 1.f);
				uniform.step          = glm::vec4(pipeline.probeDistance);

				uniform.maxDistance    = pipeline.probeDistance * 1.5f;
				uniform.depthSharpness = pipeline.depthSharpness;
				uniform.hysteresis     = pipeline.hysteresis;
				uniform.normalBias     = pipeline.normalBias;

				uniform.energyPreservation      = pipeline.energyPreservation;
				uniform.irradianceTextureWidth  = (IrradianceOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
				uniform.irradianceTextureHeight = (IrradianceOctSize + 2) * uniform.probeCounts.z + 2;
				uniform.depthTextureWidth       = (DepthOctSize + 2) * uniform.probeCounts.x * uniform.probeCounts.y + 2;
				uniform.depthTextureHeight      = (DepthOctSize + 2) * uniform.probeCounts.z + 2;

				raytracePass.samplerDescriptor->setUniform("DDGIUBO", "ddgi", &uniform);
			}

			inline auto initDDGIPipeline(ddgi::component::DDGIPipeline &pipeline, Entity entity, ecs::World world)
			{
				auto &bBox             = entity.addComponent<maple::component::BoundingBoxComponent>();
				auto &sceneBox         = world.getComponent<maple::component::BoundingBoxComponent>();
				auto &pipe             = entity.addComponent<ddgi::component::DDGIPipelineInternal>();
				auto &uniform          = entity.addComponent<ddgi::component::DDGIUniform>();
				auto &probeUpdatePass  = entity.addComponent<ddgi::component::ProbeUpdatePass>();
				auto &borderUpdatePass = entity.addComponent<ddgi::component::BorderUpdatePass>();
				auto &sampleProbePass  = entity.addComponent<ddgi::component::SampleProbePass>();

				entity.addComponent<maple::component::Transform>();
				bBox.box = sceneBox.box;

				const auto &windowSize = world.getComponent<maple::component::WindowSize>();

				float scaleDivisor = powf(2.0f, float(pipeline.scale));

				pipeline.width  = windowSize.width / scaleDivisor;
				pipeline.height = windowSize.height / scaleDivisor;

				auto &raytracePass = entity.addComponent<ddgi::component::RaytracePass>();

				raytracePass.shader = Shader::create("shaders/DDGI/GIRays.shader", {{"VertexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                    {"IndexBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                    {"SubmeshInfoBuffer", MAX_SCENE_MESH_INSTANCE_COUNT},
				                                                                    {"uSamplers", MAX_SCENE_MATERIAL_TEXTURE_COUNT}});

				{
					PipelineInfo info;
					probeUpdatePass.irradanceShader         = Shader::create("shaders/DDGI/IrradianceProbeUpdate.shader");
					info.pipelineName                       = "IrradianceProbeUpdatePipeline";
					info.shader                             = probeUpdatePass.irradanceShader;
					probeUpdatePass.irradianceProbePipeline = Pipeline::get(info);
				}

				{
					PipelineInfo info;
					probeUpdatePass.depthShader        = Shader::create("shaders/DDGI/DepthProbeUpdate.shader");
					info.pipelineName                  = "DepthProbeUpdatePipeline";
					info.shader                        = probeUpdatePass.depthShader;
					probeUpdatePass.depthProbePipeline = Pipeline::get(info);
				}

				for (uint32_t i = 0; i < 3; i++)
				{
					probeUpdatePass.irradianceDescriptors.emplace_back(DescriptorSet::create({i, probeUpdatePass.irradanceShader.get()}));
					probeUpdatePass.depthDescriptors.emplace_back(DescriptorSet::create({i, probeUpdatePass.depthShader.get()}));
				}

				{
					PipelineInfo info;
					borderUpdatePass.irradanceShader         = Shader::create("shaders/DDGI/IrradianceBorderUpdate.shader");
					info.pipelineName                        = "IrradianceBorderUpdatePipeline";
					info.shader                              = borderUpdatePass.irradanceShader;
					borderUpdatePass.irradianceProbePipeline = Pipeline::get(info);
				}

				{
					PipelineInfo info;
					borderUpdatePass.depthShader        = Shader::create("shaders/DDGI/DepthBorderUpdate.shader");
					info.pipelineName                   = "DepthBorderUpdatePipeline";
					info.shader                         = borderUpdatePass.depthShader;
					borderUpdatePass.depthProbePipeline = Pipeline::get(info);
				}

				{
					PipelineInfo info;
					sampleProbePass.shader   = Shader::create("shaders/DDGI/SampleProbe.shader");
					info.pipelineName        = "SampleProbePipeline";
					info.shader              = sampleProbePass.shader;
					sampleProbePass.pipeline = Pipeline::get(info);
					sampleProbePass.descriptors.emplace_back(DescriptorSet::create({0, sampleProbePass.shader.get()}));
				}

				borderUpdatePass.irradianceDescriptors.emplace_back(DescriptorSet::create({0, borderUpdatePass.irradanceShader.get()}));
				borderUpdatePass.depthDescriptors.emplace_back(DescriptorSet::create({0, borderUpdatePass.depthShader.get()}));

				PipelineInfo info;
				info.pipelineName         = "GIRays";
				info.shader               = raytracePass.shader;
				info.maxRayRecursionDepth = 1;
				raytracePass.pipeline     = Pipeline::get(info);


				raytracePass.samplerDescriptor  = DescriptorSet::create({5, raytracePass.shader.get()});
				raytracePass.outpuDescriptor    = DescriptorSet::create({6, raytracePass.shader.get()});

				uniformChanged(pipeline, entity, world);
				ddgi::init::initializeProbeGrid(pipeline, pipe, uniform, world);
			}
		}        // namespace delegates

		auto registerDDGI(ExecuteQueue &begin, ExecuteQueue &render, std::shared_ptr<ExecutePoint> executePoint) -> void
		{
			executePoint->addDependency<ddgi::component::DDGIPipeline, ddgi::component::DDGIVisualization>();
			executePoint->onConstruct<ddgi::component::DDGIPipeline, delegates::initDDGIPipeline>();
			executePoint->onUpdate<ddgi::component::DDGIPipeline, delegates::uniformChanged>();
			executePoint->registerWithinQueue<trace_rays::system>(render);
			executePoint->registerWithinQueue<probe_update::system>(render);
			executePoint->registerWithinQueue<border_update::system>(render);
			executePoint->registerWithinQueue<sample_probe::system>(render);
			executePoint->registerWithinQueue<end_frame::system>(render);
		}
	}        // namespace ddgi
}        // namespace maple