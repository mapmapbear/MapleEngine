//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#include "Renderer.h"
#include "Application.h"
#include "Engine/Mesh.h"

namespace maple
{
	auto Renderer::bindDescriptorSets(Pipeline *pipeline, CommandBuffer *cmdBuffer, uint32_t dynamicOffset, const std::vector<std::shared_ptr<DescriptorSet>> &descriptorSets) -> void
	{
		Application::getRenderDevice()->bindDescriptorSets(pipeline, cmdBuffer, dynamicOffset, descriptorSets);
	}

	auto Renderer::drawIndexed(CommandBuffer *commandBuffer, DrawType type, uint32_t count, uint32_t start) -> void
	{
		Application::getRenderDevice()->drawIndexed(commandBuffer, type, count, start);
	}

	auto Renderer::drawArrays(CommandBuffer* commandBuffer, DrawType type, uint32_t count, uint32_t start /*= 0*/) -> void
	{
		Application::getRenderDevice()->drawArrays(commandBuffer, type, count, start);
	}

	auto Renderer::dispatch(CommandBuffer* commandBuffer, uint32_t x, uint32_t y, uint32_t z) -> void
	{
		Application::getRenderDevice()->dispatch(commandBuffer, x, y, z);
	}

	auto Renderer::drawMesh(CommandBuffer *cmdBuffer, Pipeline *pipeline, Mesh *mesh) -> void
	{
		mesh->getVertexBuffer()->bind(cmdBuffer, pipeline);
		mesh->getIndexBuffer()->bind(cmdBuffer);
		Application::getRenderDevice()->drawIndexed(cmdBuffer, DrawType::Triangle, mesh->getIndexBuffer()->getCount());
		mesh->getVertexBuffer()->unbind();
		mesh->getIndexBuffer()->unbind();
	}
};        // namespace maple
