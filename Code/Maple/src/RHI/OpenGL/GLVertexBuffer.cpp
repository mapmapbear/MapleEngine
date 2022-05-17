
#include "GLVertexBuffer.h"
#include "GLPipeline.h"
#include "GL.h"
#include "Engine/Profiler.h"
#include "Others/Console.h"

#include "RHI/BufferUsage.h"

namespace maple
{
	namespace
	{
		inline auto bufferUsageToOpenGL(const BufferUsage &usage)
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
    }


    GLVertexBuffer::GLVertexBuffer(BufferUsage usage)
        : usage(usage)
    {
        PROFILE_FUNCTION();
        GLCall(glGenBuffers(1, &handle));
    }

    GLVertexBuffer::~GLVertexBuffer()
    {
        PROFILE_FUNCTION();
        GLCall(glDeleteBuffers(1, &handle));
    }

    auto GLVertexBuffer::resize(uint32_t size) -> void
    {
        PROFILE_FUNCTION();
        this->size = size;
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, handle));
        GLCall(glBufferData(GL_ARRAY_BUFFER, size, nullptr, bufferUsageToOpenGL(usage)));
    }

    auto GLVertexBuffer::setData(uint32_t size, const void* data) -> void
    {
        PROFILE_FUNCTION();
        this->size = size;
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, handle));
        GLCall(glBufferData(GL_ARRAY_BUFFER, size, data,bufferUsageToOpenGL(usage)));
    }

    auto GLVertexBuffer::setDataSub(uint32_t size, const void* data, uint32_t offset) -> void
    {
        PROFILE_FUNCTION();
        this->size = size;
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, handle));
        GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
    }

    auto GLVertexBuffer::getPointerInternal() -> void*
    {
        PROFILE_FUNCTION();
        
        void* result = nullptr;
        if(!mapped)
        {
            GLCall(result = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
            mapped = true;
        }
        else
        {
            LOGW("Vertex buffer already mapped");
        }

        return result;
    }

    auto GLVertexBuffer::releasePointer() -> void
    {
        PROFILE_FUNCTION();
        if(mapped)
        {
            GLCall(glUnmapBuffer(GL_ARRAY_BUFFER));
            mapped = false;
        }
    }

    auto GLVertexBuffer::bind(const CommandBuffer* commandBuffer, Pipeline* pipeline) -> void
    {
        PROFILE_FUNCTION();
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, handle));
        ((GLPipeline*)pipeline)->bindVertexArray();
    }

    auto GLVertexBuffer::unbind() -> void
    {
        PROFILE_FUNCTION();
        GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
    }
}
