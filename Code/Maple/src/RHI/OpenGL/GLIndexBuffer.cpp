#include "GLIndexBuffer.h"
#include "Engine/Profiler.h"
#include "GL.h"
#include "Others/Console.h"

namespace Maple
{
	static auto bufferUsageToOpenGL(const BufferUsage &usage) -> uint32_t
	{
		switch (usage)
		{
			case BufferUsage::Static:
				return GL_STATIC_DRAW;
			case BufferUsage::Dynamic:
				return GL_DYNAMIC_DRAW;
			case BufferUsage::Stream:
				return GL_STREAM_DRAW;
		}
		return 0;
	}

	GLIndexBuffer::GLIndexBuffer(const uint16_t *data, uint32_t count, BufferUsage bufferUsage) :
	    count(count), usage(bufferUsage)
	{
		PROFILE_FUNCTION();
		GLCall(glGenBuffers(1, &handle));
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle));
		GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint16_t), data, bufferUsageToOpenGL(usage)));
	}

	GLIndexBuffer::GLIndexBuffer(const uint32_t *data, uint32_t count, BufferUsage bufferUsage) :
	    count(count), usage(bufferUsage)
	{
		PROFILE_FUNCTION();
		GLCall(glGenBuffers(1, &handle));
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle));
		GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), data, bufferUsageToOpenGL(usage)));
	}

	GLIndexBuffer::~GLIndexBuffer()
	{
		PROFILE_FUNCTION();
		GLCall(glDeleteBuffers(1, &handle));
	}

	auto GLIndexBuffer::bind(CommandBuffer *commandBuffer) const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle));
	}

	auto GLIndexBuffer::unbind() const -> void
	{
		PROFILE_FUNCTION();
		GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	}

	auto GLIndexBuffer::getCount() const -> uint32_t
	{
		return count;
	}

	auto GLIndexBuffer::getPointerInternal() -> void *
	{
		PROFILE_FUNCTION();
		void *result = nullptr;
		if (!mapped)
		{
			GLCall(result = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));
			mapped = true;
		}
		else
		{
			LOGW("Index buffer already mapped");
		}

		return result;
	}

	auto GLIndexBuffer::releasePointer() -> void
	{
		PROFILE_FUNCTION();
		if (mapped)
		{
			GLCall(glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER));
			mapped = false;
		}
	}
}        // namespace Maple
