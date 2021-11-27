//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Renderer.h"
#include "Application.h"
#include "Engine/Mesh.h"

namespace Maple
{
	auto Renderer::bindDescriptorSets(Pipeline *pipeline, CommandBuffer *cmdBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		Application::getRenderDevice()->bindDescriptorSets(pipeline, cmdBuffer, dynamicOffset, descriptorSets);
	}

	auto Renderer::drawIndexed(CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start) -> void
	{
		Application::getRenderDevice()->drawIndexed(commandBuffer, type, count, start);
	}

	auto Renderer::getCommandBuffer() -> CommandBuffer *
	{
		return Application::getGraphicsContext()->getSwapChain()->getCurrentCommandBuffer();
	}

	auto Renderer::drawMesh(CommandBuffer *cmdBuffer, Pipeline *pipeline, Mesh *mesh) -> void
	{
		mesh->getVertexBuffer()->bind(cmdBuffer, pipeline);
		mesh->getIndexBuffer()->bind(cmdBuffer);
		Application::getRenderDevice()->drawIndexed(cmdBuffer, DrawType::Triangle, mesh->getIndexBuffer()->getCount());
		mesh->getVertexBuffer()->unbind();
		mesh->getIndexBuffer()->unbind();
	}
};        // namespace Maple
