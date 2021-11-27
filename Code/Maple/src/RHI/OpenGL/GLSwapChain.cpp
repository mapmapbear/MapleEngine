#include "GLSwapChain.h"
#include "GLTexture.h"
#include "Others/Console.h"
#include "RHI/FrameBuffer.h"

#include "GL.h"

#ifndef PLATFORM_MOBILE
#	include <GLFW/glfw3.h>
#endif


namespace Maple
{
	namespace
	{
		static std::string getStringForType(GLenum type)
		{
			switch (type)
			{
				case GL_DEBUG_TYPE_ERROR:
					return "Error";
				case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
					return "Deprecated behavior";
				case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
					return "Undefined behavior";
				case GL_DEBUG_TYPE_PORTABILITY:
					return "Portability issue";
				case GL_DEBUG_TYPE_PERFORMANCE:
					return "Performance issue";
				case GL_DEBUG_TYPE_MARKER:
					return "Stream annotation";
				case GL_DEBUG_TYPE_OTHER_ARB:
					return "Other";
				default:
					return "";
			}
		}

		static bool printMessage(GLenum type)
		{
			switch (type)
			{
				case GL_DEBUG_TYPE_ERROR:
					return true;
				case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
					return true;
				case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
					return true;
				case GL_DEBUG_TYPE_PORTABILITY:
					return true;
				case GL_DEBUG_TYPE_PERFORMANCE:
					return false;
				case GL_DEBUG_TYPE_MARKER:
					return false;
				case GL_DEBUG_TYPE_OTHER_ARB:
					return false;
				default:
					return false;
			}
		}

		static std::string getStringForSource(GLenum source)
		{
			switch (source)
			{
				case GL_DEBUG_SOURCE_API:
					return "API";
				case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
					return "Window System";
				case GL_DEBUG_SOURCE_SHADER_COMPILER:
					return "Shader compiler";
				case GL_DEBUG_SOURCE_THIRD_PARTY:
					return "Third party";
				case GL_DEBUG_SOURCE_APPLICATION:
					return "Application";
				case GL_DEBUG_SOURCE_OTHER:
					return "Other";
				default:
					return "";
			}
		}

		static std::string getStringForSeverity(GLenum severity)
		{
			switch (severity)
			{
				case GL_DEBUG_SEVERITY_HIGH:
					return "High";
				case GL_DEBUG_SEVERITY_MEDIUM:
					return "Medium";
				case GL_DEBUG_SEVERITY_LOW:
					return "Low";
				case GL_DEBUG_SEVERITY_NOTIFICATION:
				case GL_DEBUG_SOURCE_API:
					return "Source API";
				default:
					return ("");
			}
		}

		void APIENTRY openglCallbackFunction(GLenum      source,
		                                     GLenum      type,
		                                     GLuint      id,
		                                     GLenum      severity,
		                                     GLsizei     length,
		                                     const char *message,
		                                     const void *userParam)
		{
			if (!printMessage(type))
				return;

			LOGI("Message: {0}", message);
			LOGI("Type: {0}", getStringForType(type));
			LOGI("Source: {0}", getStringForSource(source));
			LOGI("ID: {0}", id);
			LOGI("Severity: {0}", getStringForSeverity(source));
		}
	}        // namespace

	GLSwapChain::GLSwapChain(uint32_t width, uint32_t height) :
	    width(width), height(height)
	{
	}

	GLSwapChain::~GLSwapChain()
	{
	}

	auto GLSwapChain::init(bool vsync) -> bool
	{
		LOGI("{0}", glGetString(GL_VERSION));
		LOGI("{0}", glGetString(GL_VENDOR));
		LOGI("{0}", glGetString(GL_RENDERER));

		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(openglCallbackFunction, nullptr);
		GLuint unusedIds = 0;
		glDebugMessageControl(GL_DONT_CARE,
		                      GL_DONT_CARE,
		                      GL_DONT_CARE,
		                      0,
		                      &unusedIds,
		                      true);

		return false;
	}

	auto GLSwapChain::getCurrentImage() -> std::shared_ptr<Texture>
	{
		return nullptr;
	}

	auto GLSwapChain::getCurrentBufferIndex() const -> uint32_t
	{
		return 0;
	}

	auto GLSwapChain::getSwapChainBufferCount() const -> size_t
	{
		return 1;
	}
}        // namespace Maple
