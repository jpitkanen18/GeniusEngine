#pragma once
// Vulkan Context Implementation
// Uses GLFW for window management + Vulkan for rendering

#include "../GAPI.hpp"
#include <vulkan/vulkan.h>
#include <vector>

struct GLFWwindow;

namespace GE::Graphics::Vulkan
{

	class VulkanContext : public IContext
	{
	public:
		VulkanContext() = default;
		~VulkanContext() override { shutdown(); }

		bool init(int width, int height, const std::string &title) override;
		void shutdown() override;
		void beginFrame() override;
		void endFrame() override;
		bool shouldClose() override;
		void setClearColor(const Color &color) override;
		void setViewport(int x, int y, int width, int height) override;
		void setDepthTest(bool enabled) override;
		void setBlending(bool enabled) override;
		void *getNativeWindow() override { return this->m_window; }

		std::string getDeviceName() const override { return this->m_deviceName; }
		std::string getDriverVersion() const override { return this->m_driverVersion; }
		std::string getAPIVersion() const override { return this->m_apiVersion; }

		GLFWwindow *getGLFWWindow() { return this->m_window; }
		int getWidth() const { return this->m_width; }
		int getHeight() const { return this->m_height; }

	private:
		GLFWwindow *m_window = nullptr;
		int m_width = 0;
		int m_height = 0;
		Color m_clearColor{0.02f, 0.02f, 0.05f, 1.0f};

		VkInstance m_instance = VK_NULL_HANDLE;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_device = VK_NULL_HANDLE;
		VkQueue m_graphicsQueue = VK_NULL_HANDLE;
		VkQueue m_presentQueue = VK_NULL_HANDLE;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
		VkRenderPass m_renderPass = VK_NULL_HANDLE;
		VkCommandPool m_commandPool = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

		std::vector<VkImage> m_swapchainImages;
		std::vector<VkImageView> m_swapchainImageViews;
		std::vector<VkFramebuffer> m_framebuffers;
		std::vector<VkCommandBuffer> m_commandBuffers;
		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;

		VkImage m_depthImage = VK_NULL_HANDLE;
		VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
		VkImageView m_depthImageView = VK_NULL_HANDLE;

		VkFormat m_swapchainFormat = VK_FORMAT_B8G8R8A8_SRGB;
		VkExtent2D m_swapchainExtent{};
		uint32_t m_graphicsFamily = 0;
		uint32_t m_presentFamily = 0;

		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
		uint32_t m_currentFrame = 0;
		uint32_t m_currentImageIndex = 0;

		bool m_depthTestEnabled = true;
		bool m_blendingEnabled = true;

		bool createInstance();
		bool pickPhysicalDevice();
		bool createLogicalDevice();
		bool createSwapchain();
		bool createRenderPass();
		bool createFramebuffers();
		bool createCommandPool();
		bool createCommandBuffers();
		bool createSyncObjects();
		bool createDepthResources();
		void cleanupSwapchain();

		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		std::string m_deviceName;
		std::string m_driverVersion;
		std::string m_apiVersion;
	};

} // namespace GE::Graphics::Vulkan
