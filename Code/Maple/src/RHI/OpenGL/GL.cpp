#include "GL.h"
#include "Others/Console.h"

namespace Maple
{
	auto logCall(const char *function, const char *file, const int32_t line) -> bool
	{
		GLenum err(glGetError());
		bool   ret = true;
		while (err != GL_NO_ERROR)
		{
			std::string error;
			switch (err)
			{
#define STR(r)      \
	case GL_##r:    \
		error = #r; \
		break;
				STR(INVALID_OPERATION);
				STR(INVALID_ENUM);
				STR(INVALID_VALUE);
				STR(OUT_OF_MEMORY);
				STR(INVALID_FRAMEBUFFER_OPERATION);
#undef STR
				default:
					return "UNKNOWN_GL_ERROR";
			}
			LOGE("Error : {0}, File : {1}, Func : {2}, Line : {3}", error, file, function, line);
			ret = false;
			err = glGetError();
		}
		return ret;
	}

	auto clearError() -> void
	{
		while (glGetError() != GL_NO_ERROR)
			;
	}
}        // namespace Maple
