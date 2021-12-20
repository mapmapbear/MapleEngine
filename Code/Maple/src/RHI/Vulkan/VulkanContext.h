//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#pragma once
#include "Engine/Core.h"
#include "RHI/GraphicsContext.h"
#include <deque>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace maple
{
	class MAPLE_EXPORT VulkanContext : public GraphicsContext
	{
	  public:
		VulkanContext();
		~VulkanContext();

		auto init() -> void override;
		auto present() -> void override;
		auto getMinUniformBufferOffsetAlignment() const -> size_t override;
		auto waitIdle() const -> void override;
		auto onImGui() -> void override;

		inline auto getGPUMemoryUsed() -> float override
		{
			return 0;
		}

		inline auto getTotalGPUMemory() -> float override
		{
			return 0;
		}

		inline const auto getVkInstance() const
		{
			return vkInstance;
		}
		inline auto getVkInstance()
		{
			return vkInstance;
		}

		struct DeletionQueue
		{
			DeletionQueue()                      = default;
			DeletionQueue(const DeletionQueue &) = delete;
			auto operator=(const DeletionQueue &) -> DeletionQueue & = delete;

			std::deque<std::function<void()>> deletors;

			template <typename F>
			inline auto emplace(F &&function)
			{
				deletors.emplace_back(function);
			}

			inline auto flush()
			{
				for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
				{
					(*it)();
				}
				deletors.clear();
			}
		};

		static auto get() -> std::shared_ptr<VulkanContext>;

		static auto getDeletionQueue() -> DeletionQueue &;
		static auto getDeletionQueue(uint32_t index) -> DeletionQueue &;

	  private:
		auto setupDebug() -> void;

		VkInstance vkInstance;

		//bind to triple buffer 
		DeletionQueue deletionQueue[3];

		std::vector<const char *>          instanceLayerNames;
		std::vector<const char *>          instanceExtensionNames;
		std::vector<VkLayerProperties>     instanceLayers;
		std::vector<VkExtensionProperties> instanceExtensions;

		auto createInstance() -> void;
	};

};        // namespace maple
