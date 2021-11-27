//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "RenderDevice.h"
#include "Application.h"

#include "RHI/OpenGL/GLRenderDevice.h"

namespace Maple
{
	auto RenderDevice::clear(uint32_t bufferMask) -> void
	{
		Application::getRenderDevice()->clearInternal(bufferMask);
	}

	auto RenderDevice::present() -> void
	{
		Application::getRenderDevice()->presentInternal();
	}

	auto RenderDevice::present(CommandBuffer *commandBuffer) -> void
	{
		Application::getRenderDevice()->presentInternal(commandBuffer);
	}

	auto RenderDevice::bindDescriptorSets(Pipeline *pipeline, CommandBuffer *commandBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		Application::getRenderDevice()->bindDescriptorSetsInternal(pipeline, commandBuffer, dynamicOffset, descriptorSets);
	}

	auto RenderDevice::draw(CommandBuffer *commandBuffer, DrawType type, uint32_t count, DataType datayType, const void *indices) -> void
	{
		Application::getRenderDevice()->drawInternal(commandBuffer, type, count, datayType, indices);
	}

	auto RenderDevice::drawIndexed(CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start) -> void
	{
		Application::getRenderDevice()->drawIndexedInternal(commandBuffer, type, count, start);
	}

	auto RenderDevice::create() -> std::shared_ptr<RenderDevice>
	{
		return std::make_shared<GLRenderDevice>();
	}
}        // namespace Maple
