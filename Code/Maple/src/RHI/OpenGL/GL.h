#pragma once

#ifdef PLATFORM_MOBILE
#	ifdef PLATFORM_IOS
#		include <OpenGLES/ES3/gl.h>
#		include <OpenGLES/ES3/glext.h>
#	elif PLATFORM_ANDROID
#		include <GLES3/gl3.h>
#	endif
#else
#	include <glad/glad.h>
#endif
#include <cstdint>

namespace Maple
{
#ifdef MAPLE_DEBUG
#	ifdef glDebugMessageCallback
#		define GL_DEBUD_CALLBACK 1
#	else
#		define GL_DEBUG 1
#	endif
#endif

	auto logCall(const char *function, const char *file, const int32_t line) -> bool;
	auto clearError() -> void;

};        // namespace Maple

#define GL_DEBUG 1

#if GL_DEBUG
#	define GLCall(x)                                \
		Maple::clearError();                         \
		x;                                           \
		if (!Maple::logCall(#x, __FILE__, __LINE__)) \
		{}//	__debugbreak();
#else
#	define GLCall(x) x
#endif
