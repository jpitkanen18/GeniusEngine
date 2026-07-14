#pragma once
// Vulkan backend global state (instance, device, etc.)
// Set by VulkanContext, consumed by VulkanShader/Buffer/Texture

#include <vulkan/vulkan.h>

namespace GE::Graphics::Vulkan
{

	class VulkanState
	{
	public:
		static void setInstance(VkInstance inst) { s_instance = inst; }
		static VkInstance getInstance() { return s_instance; }

		static void setPhysicalDevice(VkPhysicalDevice dev) { s_physicalDevice = dev; }
		static VkPhysicalDevice getPhysicalDevice() { return s_physicalDevice; }

		static void setDevice(VkDevice dev) { s_device = dev; }
		static VkDevice getDevice() { return s_device; }

		static void setGraphicsQueue(VkQueue q) { s_graphicsQueue = q; }
		static VkQueue getGraphicsQueue() { return s_graphicsQueue; }

		static void setCommandPool(VkCommandPool pool) { s_commandPool = pool; }
		static VkCommandPool getCommandPool() { return s_commandPool; }

		static void setRenderPass(VkRenderPass rp) { s_renderPass = rp; }
		static VkRenderPass getRenderPass() { return s_renderPass; }

		static void setCurrentCommandBuffer(VkCommandBuffer cb) { s_currentCmdBuf = cb; }
		static VkCommandBuffer getCurrentCommandBuffer() { return s_currentCmdBuf; }

		static void setGraphicsQueueFamily(uint32_t idx) { s_graphicsFamily = idx; }
		static uint32_t getGraphicsQueueFamily() { return s_graphicsFamily; }

		static void setExtent(VkExtent2D ext) { s_extent = ext; }
		static VkExtent2D getExtent() { return s_extent; }

	private:
		static inline VkInstance s_instance = VK_NULL_HANDLE;
		static inline VkPhysicalDevice s_physicalDevice = VK_NULL_HANDLE;
		static inline VkDevice s_device = VK_NULL_HANDLE;
		static inline VkQueue s_graphicsQueue = VK_NULL_HANDLE;
		static inline VkCommandPool s_commandPool = VK_NULL_HANDLE;
		static inline VkRenderPass s_renderPass = VK_NULL_HANDLE;
		static inline VkCommandBuffer s_currentCmdBuf = VK_NULL_HANDLE;
		static inline uint32_t s_graphicsFamily = 0;
		static inline VkExtent2D s_extent = {0, 0};
	};

} // namespace GE::Graphics::Vulkan
