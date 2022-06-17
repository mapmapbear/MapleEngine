//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "RHI/UniformBuffer.h"

namespace maple
{
	class GLShader;

	class GLUniformBuffer : public UniformBuffer
	{
	  public:
		GLUniformBuffer();
		~GLUniformBuffer();

		auto init(uint32_t size, const void *data) -> void override;
		auto setData(uint32_t size, const void *data) -> void override;
		auto setDynamicData(uint32_t size, uint32_t typeSize, const void *data) -> void override;
		auto bind(uint32_t slot, GLShader *shader, const std::string &name) -> void;

		inline auto setData(const void *data) -> void override
		{
			setData(size, data);
		}

		inline auto getBuffer() const -> uint8_t * override
		{
			return data;
		};

		inline auto getSize() const
		{
			return size;
		}
		inline auto getTypeSize() const
		{
			return dynamicTypeSize;
		}
		inline auto isDynamic() const
		{
			return dynamic;
		}

		inline auto getHandle() const
		{
			return handle;
		}

	  private:
		uint8_t *data            = nullptr;
		uint32_t size            = 0;
		uint32_t dynamicTypeSize = 0;
		bool     dynamic         = false;
		uint32_t handle          = 0;
		uint32_t index           = 0;
	};
}        // namespace maple
