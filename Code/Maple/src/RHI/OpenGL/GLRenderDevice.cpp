#include "GLRenderDevice.h"
#include "GLContext.h"
#include "GLSwapChain.h"

#include "Engine/Core.h"
#include "Engine/Profiler.h"
#include "Others/Console.h"

#include "GL.h"
#include "GLDescriptorSet.h"
#include "GLFramebuffer.h"

#include "RHI/Texture.h"

#include "Application.h"

namespace maple
{
	namespace
	{
		inline auto rendererBufferToGL(uint32_t buffer) -> uint32_t
		{
			uint32_t result = 0;
			if (buffer & RendererBufferColor)
				result |= GL_COLOR_BUFFER_BIT;
			if (buffer & RendererBufferDepth)
				result |= GL_DEPTH_BUFFER_BIT;
			if (buffer & RendererBufferStencil)
				result |= GL_STENCIL_BUFFER_BIT;
			return result;
		}

		inline auto rendererBlendFunctionToGL(RendererBlendFunction function) -> uint32_t
		{
			switch (function)
			{
				case RendererBlendFunction::Zero:
					return GL_ZERO;
				case RendererBlendFunction::One:
					return GL_ONE;
				case RendererBlendFunction::SourceAlpha:
					return GL_SRC_ALPHA;
				case RendererBlendFunction::DestinationAlpha:
					return GL_DST_ALPHA;
				case RendererBlendFunction::OneMinusSourceAlpha:
					return GL_ONE_MINUS_SRC_ALPHA;
				default:
					return 0;
			}
		}

		inline auto stencilTypeToGL(const StencilType type) -> uint32_t
		{
			switch (type)
			{
				case StencilType::Equal:
					return GL_EQUAL;
				case StencilType::Notequal:
					return GL_NOTEQUAL;
				case StencilType::Keep:
					return GL_KEEP;
				case StencilType::Replace:
					return GL_REPLACE;
				case StencilType::Zero:
					return GL_ZERO;
				case StencilType::Always:
					return GL_ALWAYS;
				default:
					MAPLE_ASSERT(false, "[Texture] Unsupported StencilType");
					return 0;
			}
		}

		inline auto drawTypeToGL(DrawType drawType) -> uint32_t
		{
			switch (drawType)
			{
				case DrawType::Point:
					return GL_POINTS;
				case DrawType::Lines:
					return GL_LINES;
				case DrawType::Triangle:
					return GL_TRIANGLES;
				default:
					MAPLE_ASSERT(false, "Unsupported DrawType");
					break;
			}
			return 0;
		}

		inline auto dataTypeToGL(DataType dataType) -> uint32_t
		{
			switch (dataType)
			{
				case DataType::Float:
					return GL_FLOAT;
				case DataType::UnsignedInt:
					return GL_UNSIGNED_INT;
				case DataType::UnsignedByte:
					return GL_UNSIGNED_BYTE;
				default:
					MAPLE_ASSERT(false, "Unsupported DataType");
					break;
			}
			return 0;
		}
	}        // namespace

	GLRenderDevice::GLRenderDevice()
	{
		/*auto &caps = getRenderAPI();

		caps.vendor   = (const char *) glGetString(GL_VENDOR);
		caps.renderer = (const char *) glGetString(GL_RENDERER);
		caps.version  = (const char *) glGetString(GL_VERSION);

		glGetIntegerv(GL_MAX_SAMPLES, &caps.maxSamples);
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &caps.maxTextureUnits);
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &caps.uniformBufferOffsetAlignment);*/
	}

	GLRenderDevice::~GLRenderDevice()
	{
	}

	auto GLRenderDevice::init() -> void
	{
		PROFILE_FUNCTION();
		auto vendor   = (const char *) glGetString(GL_VENDOR);
		auto renderer = (const char *) glGetString(GL_RENDERER);
		auto version  = (const char *) glGetString(GL_VERSION);

		LOGI("Vendor {0}\n, Renderer {1}\n, Version {2}", vendor, renderer, version);

		GLCall(glEnable(GL_DEPTH_TEST));
		GLCall(glEnable(GL_STENCIL_TEST));
		GLCall(glEnable(GL_CULL_FACE));
		GLCall(glEnable(GL_BLEND));
		GLCall(glDepthFunc(GL_LEQUAL));
		GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		GLCall(glBlendEquation(GL_FUNC_ADD));

#ifndef PLATFORM_MOBILE
		GLCall(glEnable(GL_DEPTH_CLAMP));
		GLCall(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
#endif
	}

	auto GLRenderDevice::begin() -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
		GLCall(glClearColor(0, 0, 0, 1));
		GLCall(glClear(GL_COLOR_BUFFER_BIT));
	}

	auto GLRenderDevice::clearInternal(uint32_t bufferMask) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glClear(rendererBufferToGL(bufferMask)));
	}

	auto GLRenderDevice::dispatch(CommandBuffer *commandBuffer, uint32_t x, uint32_t y, uint32_t z) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glDispatchCompute(x,y,z));
	}

	auto GLRenderDevice::memoryBarrier(CommandBuffer* commandBuffer,MemoryBarrierFlags flag) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
	}

	auto GLRenderDevice::presentInternal() -> void
	{
	}

	auto GLRenderDevice::presentInternal(CommandBuffer *commandBuffer) -> void
	{
	}


	auto GLRenderDevice::setDepthTestingInternal(bool enabled) -> void
	{
		PROFILE_FUNCTION();
		if (enabled)
		{
			GLCall(glEnable(GL_DEPTH_TEST));
		}
		else
		{
			GLCall(glDisable(GL_DEPTH_TEST));
		}
	}

	auto GLRenderDevice::setDepthMaskInternal(bool enabled) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glDepthMask(enabled ? GL_TRUE : GL_FALSE));
	}

	auto GLRenderDevice::setPixelPackType(const PixelPackType type) -> void
	{
		PROFILE_FUNCTION();
		switch (type)
		{
			case PixelPackType::Pack:
				GLCall(glPixelStorei(GL_PACK_ALIGNMENT, 1));
				break;
			case PixelPackType::UnPack:
				GLCall(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
				break;
		}
	}

	auto GLRenderDevice::setBlendInternal(bool enabled) -> void
	{
		PROFILE_FUNCTION();
		if (enabled)
		{
			GLCall(glEnable(GL_BLEND));
		}
		else
		{
			GLCall(glDisable(GL_BLEND));
		}
	}

	auto GLRenderDevice::setBlendFunctionInternal(RendererBlendFunction source, RendererBlendFunction destination) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBlendFunc(rendererBlendFunctionToGL(source), rendererBlendFunctionToGL(destination)));
	}

	auto GLRenderDevice::setBlendEquationInternal(RendererBlendFunction blendEquation) -> void
	{
	}

	auto GLRenderDevice::setStencilMaskInternal(uint32_t mask) -> void
	{
		glStencilMask(mask);
	}

	auto GLRenderDevice::setViewportInternal(uint32_t x, uint32_t y, uint32_t width, uint32_t height) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glViewport(x, y, width, height));
	}

	auto GLRenderDevice::setRenderModeInternal(RenderMode mode) -> void
	{
		PROFILE_FUNCTION();
#ifndef PLATFORM_MOBILE
		switch (mode)
		{
			case RenderMode::Fill:
				GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
				break;
			case RenderMode::Wireframe:
				GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
				break;
		}
#endif
	}

	auto GLRenderDevice::onResize(uint32_t width, uint32_t height) -> void
	{
		std::static_pointer_cast<GLSwapChain>(Application::getGraphicsContext()->getSwapChain())->onResize(width, height);
	}

	auto GLRenderDevice::setCullingInternal(bool enabled, bool front) -> void
	{
		PROFILE_FUNCTION();
		if (enabled)
		{
			GLCall(glEnable(GL_CULL_FACE));
			GLCall(glCullFace(front ? GL_FRONT : GL_BACK));
		}
		else
		{
			GLCall(glDisable(GL_CULL_FACE));
		}
	}

	auto GLRenderDevice::setStencilTestInternal(bool enabled) -> void
	{
		PROFILE_FUNCTION();
		if (enabled)
		{
			GLCall(glEnable(GL_STENCIL_TEST));
		}
		else
		{
			GLCall(glDisable(GL_STENCIL_TEST));
		}
	}

	auto GLRenderDevice::setStencilFunctionInternal(const StencilType type, uint32_t ref, uint32_t mask) -> void
	{
		PROFILE_FUNCTION();
		glStencilFunc(stencilTypeToGL(type), ref, mask);
	}

	auto GLRenderDevice::setStencilOpInternal(const StencilType fail, const StencilType zfail, const StencilType zpass) -> void
	{
		PROFILE_FUNCTION();
		glStencilOp(stencilTypeToGL(fail), stencilTypeToGL(zfail), stencilTypeToGL(zpass));
	}

	auto GLRenderDevice::setColorMaskInternal(bool r, bool g, bool b, bool a) -> void
	{
		PROFILE_FUNCTION();
		glColorMask(r, g, b, a);
	}

	auto GLRenderDevice::drawInternal(CommandBuffer *commandBuffer, const DrawType type, uint32_t count, DataType dataType, const void *indices) const -> void
	{
		PROFILE_FUNCTION();
		//NumDrawCalls++;
		GLCall(glDrawElements(drawTypeToGL(type), count, dataTypeToGL(dataType), indices));
	}

	auto GLRenderDevice::drawIndexedInternal(CommandBuffer *commandBuffer, const DrawType type, uint32_t count, uint32_t start) const -> void
	{
		PROFILE_FUNCTION();
		//NumDrawCalls++;
		GLCall(glDrawElements(drawTypeToGL(type), count, dataTypeToGL(DataType::UnsignedInt), nullptr));
	}

	auto GLRenderDevice::drawArraysInternal(CommandBuffer* commandBuffer, DrawType type, uint32_t count, uint32_t start /*= 0*/) const -> void
	{
		PROFILE_FUNCTION();
		//NumDrawCalls++;
		GLCall(glDrawArrays(drawTypeToGL(type), start, count));
	}

	auto GLRenderDevice::bindDescriptorSetsInternal(Pipeline *pipeline, CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		PROFILE_FUNCTION();
		for (auto descriptor : descriptorSets)
		{
			if (descriptor)
				std::static_pointer_cast<GLDescriptorSet>(descriptor)->bind(dynamicOffset);
		}
	}

	auto GLRenderDevice::clearRenderTarget(const std::shared_ptr<Texture> &texture, CommandBuffer *commandBuffer, const glm::vec4 &clearColor) -> void
	{
		if (texture == nullptr)
		{
			//Assume swapchain texture
			//TODO: function for clearing swapchain image
			GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		}
		else
		{
			GLCall(glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w));

			FrameBufferInfo frameBufferDesc{};
			frameBufferDesc.width       = texture->getWidth();
			frameBufferDesc.height      = texture->getHeight();
			frameBufferDesc.renderPass  = nullptr;
			frameBufferDesc.attachments = {texture};
			auto frameBuffer            = FrameBuffer::create(frameBufferDesc);
			frameBuffer->bind();
		}
		clear(RendererBufferColor | RendererBufferDepth | RendererBufferStencil);
	}

}        // namespace maple
