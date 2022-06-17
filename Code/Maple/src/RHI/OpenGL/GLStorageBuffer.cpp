//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#include "GL.h"
#include "GLStorageBuffer.h"
#include "Engine/Core.h"
#include "Engine/Profiler.h"
#include "Others/Console.h"

namespace maple
{
	GLStorageBuffer::GLStorageBuffer()
	{
		PROFILE_FUNCTION();
		glGenBuffers(1, &handle);
	}

	GLStorageBuffer::GLStorageBuffer(uint32_t size, const void* data)
		:GLStorageBuffer() 
	{
		this->size = size;
		GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle));
		GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY));
	}

	GLStorageBuffer::~GLStorageBuffer()
	{
		PROFILE_FUNCTION();
		GLCall(glDeleteBuffers(1, &handle));
	}

	auto GLStorageBuffer::setData(uint32_t size, const void* data) -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle));
		if (this->size == 0) 
		{
			this->size = size;
			GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_COPY));
		}
		else 
		{
			PROFILE_SCOPE("glMapBuffer");
			auto p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
			memcpy(p, data, size);
			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
	}

	auto GLStorageBuffer::bind(uint32_t slot)const -> void
	{
		PROFILE_FUNCTION();
		glBindBuffer(GL_UNIFORM_BUFFER, handle);
		GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, handle));
	}

	auto GLStorageBuffer::unbind() const-> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, handle));
	}

}
