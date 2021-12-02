//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <vector>
#include "RHI/Vulkan/VulkanHelper.h"

struct ImGui_ImplVulkanH_Window;
namespace maple 
{

	class VulkanCommandBuffer;
	class VulkanFrameBuffer;
	class VulkanRenderPass;
	class VulkanTexture2D;


	class VkImGUIRenderer 
	{
	public:
		VkImGUIRenderer(uint32_t width, uint32_t height, bool clearScreen);
		~VkImGUIRenderer();

		auto init() -> void;
		auto newFrame() -> void;
		auto render(VulkanCommandBuffer* commandBuffer) -> void;
		auto onResize(uint32_t width, uint32_t height) -> void;
		auto clear() -> void;
		
		auto frameRender(ImGui_ImplVulkanH_Window* wd) -> void;
		auto setupVulkanWindowData(ImGui_ImplVulkanH_Window* wd,VkSurfaceKHR surface, int32_t width, int32_t height) -> void;
		auto rebuildFontTexture() -> void;
	private:
		void* windowHandle = nullptr;
	
		std::shared_ptr<VulkanCommandBuffer> commandBuffers[3];
		std::vector<std::shared_ptr<VulkanFrameBuffer>> frameBuffers;

		std::shared_ptr<VulkanRenderPass> renderPass;
		std::shared_ptr<VulkanTexture2D> fontTexture;
		


		uint32_t width;
		uint32_t height;
		bool clearScreen;
	};
	
};