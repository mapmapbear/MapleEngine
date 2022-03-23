#include "GL.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace maple
{
	inline auto setUniform1f(uint32_t location, float value) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniform1f(location, value));
	}
	inline auto setUniform1fv(uint32_t location, float *value, int32_t count) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniform1fv(location, count, value));
	}
	inline auto setUniform1i(uint32_t location, int32_t value) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniform1i(location, value));
	}
	inline auto setUniform1ui(uint32_t location, uint32_t value) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniform1ui(location, value));
	}
	inline auto setUniform1iv(uint32_t location, int32_t *value, int32_t count) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniform1iv(location, count, value));
	}
	inline auto setUniform2f(uint32_t location, const glm::vec2 &vector) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniform2f(location, vector.x, vector.y));
	}
	inline auto setUniform3f(uint32_t location, const glm::vec3 &vector) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniform3f(location, vector.x, vector.y, vector.z));
	}
	inline auto setUniform4f(uint32_t location, const glm::vec4 &vector) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniform4f(location, vector.x, vector.y, vector.z, vector.w));
	}

	inline auto setUniformMat3(uint32_t location, const glm::mat3 &matrix) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(matrix)));
	}
	inline auto setUniformMat4(uint32_t location, const glm::mat4 &matrix) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix)));
	}
	inline auto setUniformMat4Array(uint32_t location, const void * buffer,uint32_t size) -> void
	{
		if (location != GL_INVALID_INDEX)
			GLCall(glUniformMatrix4fv(location, size, GL_FALSE, (const float*)buffer));
	}
};        // namespace maple
