#include "GLUniformBuffer.h"
#include "Engine/Profiler.h"
#include "GL.h"
#include "GLShader.h"

namespace maple
{
	GLUniformBuffer::GLUniformBuffer()
	{
		PROFILE_FUNCTION();
		glGenBuffers(1, &handle);
	}

	GLUniformBuffer::~GLUniformBuffer()
	{
		PROFILE_FUNCTION();
		GLCall(glDeleteBuffers(1, &handle));
	}

	auto GLUniformBuffer::init(uint32_t size, const void *data) -> void
	{
		PROFILE_FUNCTION();
		this->data = (uint8_t *) data;
		this->size = size;
		glBindBuffer(GL_UNIFORM_BUFFER, handle);
		glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
	}

	auto GLUniformBuffer::setData(uint32_t size, const void *data) -> void
	{
		PROFILE_FUNCTION();
		this->data = (uint8_t *) data;
		GLvoid *p  = nullptr;

		glBindBuffer(GL_UNIFORM_BUFFER, handle);

		if (size != this->size)
		{
			PROFILE_SCOPE("glMapBuffer");
			p          = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
			this->size = size;
			memcpy(p, data, size);
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
		else
		{
			PROFILE_SCOPE("glBufferSubData");
#ifdef PLATFORM_MACOS
			glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
#else
			glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
#endif
		}
	}

	auto GLUniformBuffer::setDynamicData(uint32_t size, uint32_t typeSize, const void *data) -> void
	{
		PROFILE_FUNCTION();
		this->data            = (uint8_t *) data;
		this->size            = size;
		this->dynamic         = true;
		this->dynamicTypeSize = typeSize;

		GLvoid *p = nullptr;

		glBindBuffer(GL_UNIFORM_BUFFER, handle);

		if (size != this->size)
		{
			PROFILE_SCOPE("glMapBuffer");
			p          = glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
			this->size = size;

			memcpy(p, data, size);
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}
		else
		{
			PROFILE_SCOPE("glBufferSubData");
			glBufferSubData(GL_UNIFORM_BUFFER, 0, size, data);
		}
	}

	auto GLUniformBuffer::bind(uint32_t slot, GLShader *shader, const std::string &name) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindBufferBase(GL_UNIFORM_BUFFER, slot, handle));
		shader->bindUniformBuffer(this, slot, name);
	}
}        // namespace maple
