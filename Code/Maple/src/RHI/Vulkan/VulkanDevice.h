//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Engine/Profiler.h"
#include "VulkanHelper.h"

#include <TracyVulkan.hpp>
#include <assert.h>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
namespace maple
{
	class VulkanCommandPool;

	class VulkanPhysicalDevice final
	{
	  public:
		VulkanPhysicalDevice();

		inline auto isExtensionSupported(const std::string &extensionName) const
		{
			return supportedExtensions.find(extensionName) != supportedExtensions.end();
		}

		auto getMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) const -> uint32_t;

		inline operator auto() const
		{
			return physicalDevice;
		}

		inline auto &getProperties() const
		{
			return physicalDeviceProperties;
		};

		inline auto &getQueueFamilyIndices() const
		{
			return indices;
		}
		inline auto &getMemoryProperties() const
		{
			return memoryProperties;
		}

	  private:
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		std::unordered_set<std::string>      supportedExtensions;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		VkPhysicalDevice                 physicalDevice;
		VkPhysicalDeviceFeatures         features;
		VkPhysicalDeviceProperties       physicalDeviceProperties;
		VkPhysicalDeviceMemoryProperties memoryProperties;

		friend class VulkanDevice;
		QueueFamilyIndices indices;
	};

	class VulkanDevice final
	{
	  public:
		VulkanDevice();
		~VulkanDevice();

		auto init() -> bool;
		auto createPipelineCache() -> void;

		inline const auto getPhysicalDevice() const
		{
			return physicalDevice;
		}
		inline auto getPhysicalDevice()
		{
			return physicalDevice;
		}
		inline auto getGraphicsQueue()
		{
			return graphicsQueue;
		}
		inline auto getPresentQueue()
		{
			return presentQueue;
		}
		inline auto getCommandPool()
		{
			return commandPool;
		}
		inline auto getPipelineCache() const
		{
			return pipelineCache;
		}

		static auto get() -> std::shared_ptr<VulkanDevice>
		{
			if (instance == nullptr)
			{
				instance = std::make_shared<VulkanDevice>();
			}
			return instance;
		}

		inline operator auto() const
		{
			return device;
		}

#if defined(MAPLE_PROFILE) && defined(TRACY_ENABLE)
		auto getTracyContext() -> tracy::VkCtx *;
#endif

	  private:
		auto createTracyContext() -> void;

		std::shared_ptr<VulkanPhysicalDevice> physicalDevice;
		std::shared_ptr<VulkanCommandPool>    commandPool;
		static std::shared_ptr<VulkanDevice>  instance;

		VkDevice device = nullptr;
		VkQueue  graphicsQueue;
		VkQueue  presentQueue;

		VkDescriptorPool         descriptorPool;
		VkPhysicalDeviceFeatures enabledFeatures;
		VkPipelineCache          pipelineCache;

#if defined(MAPLE_PROFILE) && defined(TRACY_ENABLE)
		std::vector<tracy::VkCtx *> tracyContext;
#endif

		bool enableDebugMarkers = false;
	};
};        // namespace maple
