#include "VulkanContext.hpp"
#include "VulkanState.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <set>
#include <algorithm>
#include <cstring>
#include <cstdlib>

#ifdef __APPLE__
#include <unistd.h>
#endif

namespace GE::Graphics::Vulkan
{

	bool VulkanContext::init(int width, int height, const std::string &title)
	{
		this->m_width = width;
		this->m_height = height;

#ifdef __APPLE__
		// MoltenVK requires VK_ICD_FILENAMES to find the ICD JSON.
		// If not set, try common paths.
		if (!getenv("VK_ICD_FILENAMES"))
		{
			static const char *icdPaths[] = {
				"/usr/local/share/vulkan/icd.d/MoltenVK_icd.json",
				"/opt/homebrew/share/vulkan/icd.d/MoltenVK_icd.json",
			};
			for (auto path : icdPaths)
			{
				if (access(path, R_OK) == 0)
				{
					setenv("VK_ICD_FILENAMES", path, 0);
					break;
				}
			}
		}
#endif

		if (!glfwInit())
		{
			std::cerr << "[VulkanContext] Failed to initialize GLFW\n";
			return false;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		this->m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		if (!this->m_window)
		{
			std::cerr << "[VulkanContext] Failed to create GLFW window\n";
			return false;
		}

		if (!createInstance())
			return false;

		if (glfwCreateWindowSurface(this->m_instance, this->m_window, nullptr, &this->m_surface) != VK_SUCCESS)
		{
			std::cerr << "[VulkanContext] Failed to create window surface\n";
			return false;
		}

		if (!pickPhysicalDevice())
			return false;
		if (!createLogicalDevice())
			return false;
		if (!createSwapchain())
			return false;
		if (!createRenderPass())
			return false;
		if (!createDepthResources())
			return false;
		if (!createFramebuffers())
			return false;
		if (!createCommandPool())
			return false;
		if (!createCommandBuffers())
			return false;
		if (!createSyncObjects())
			return false;

		// Publish state
		VulkanState::setInstance(this->m_instance);
		VulkanState::setPhysicalDevice(this->m_physicalDevice);
		VulkanState::setDevice(this->m_device);
		VulkanState::setGraphicsQueue(this->m_graphicsQueue);
		VulkanState::setCommandPool(this->m_commandPool);
		VulkanState::setRenderPass(this->m_renderPass);
		VulkanState::setGraphicsQueueFamily(this->m_graphicsFamily);
		VulkanState::setExtent(this->m_swapchainExtent);

		return true;
	}

	void VulkanContext::shutdown()
	{
		if (this->m_device == VK_NULL_HANDLE)
		{
			// Init failed before device creation — just clean up GLFW
			if (this->m_surface && this->m_instance)
				vkDestroySurfaceKHR(this->m_instance, this->m_surface, nullptr);
			if (this->m_instance)
				vkDestroyInstance(this->m_instance, nullptr);
			if (this->m_window)
			{
				glfwDestroyWindow(this->m_window);
				this->m_window = nullptr;
			}
			glfwTerminate();
			return;
		}

		vkDeviceWaitIdle(this->m_device); // May fail if device was lost; that's OK for shutdown

		cleanupSwapchain();

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (this->m_renderFinishedSemaphores[i])
				vkDestroySemaphore(this->m_device, this->m_renderFinishedSemaphores[i], nullptr);
			if (this->m_imageAvailableSemaphores[i])
				vkDestroySemaphore(this->m_device, this->m_imageAvailableSemaphores[i], nullptr);
			if (this->m_inFlightFences[i])
				vkDestroyFence(this->m_device, this->m_inFlightFences[i], nullptr);
		}

		if (this->m_commandPool)
			vkDestroyCommandPool(this->m_device, this->m_commandPool, nullptr);
		if (this->m_device)
			vkDestroyDevice(this->m_device, nullptr);

		// Clear state so shader destructors (called later via shared_ptr) don't use stale device
		VulkanState::setDevice(VK_NULL_HANDLE);

		if (this->m_surface)
			vkDestroySurfaceKHR(this->m_instance, this->m_surface, nullptr);
		if (this->m_instance)
			vkDestroyInstance(this->m_instance, nullptr);

		if (this->m_window)
		{
			glfwDestroyWindow(this->m_window);
			this->m_window = nullptr;
		}
		glfwTerminate();
	}

	void VulkanContext::beginFrame()
	{
		glfwPollEvents();

		vkWaitForFences(this->m_device, 1, &this->m_inFlightFences[this->m_currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(this->m_device, 1, &this->m_inFlightFences[this->m_currentFrame]);

		vkAcquireNextImageKHR(this->m_device, this->m_swapchain, UINT64_MAX,
							  this->m_imageAvailableSemaphores[this->m_currentFrame], VK_NULL_HANDLE,
							  &this->m_currentImageIndex);

		VkCommandBuffer cmd = this->m_commandBuffers[this->m_currentFrame];
		vkResetCommandBuffer(cmd, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(cmd, &beginInfo);

		VkRenderPassBeginInfo rpInfo{};
		rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rpInfo.renderPass = this->m_renderPass;
		rpInfo.framebuffer = this->m_framebuffers[this->m_currentImageIndex];
		rpInfo.renderArea.offset = {0, 0};
		rpInfo.renderArea.extent = this->m_swapchainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {{this->m_clearColor.r, this->m_clearColor.g, this->m_clearColor.b, this->m_clearColor.a}};
		clearValues[1].depthStencil = {1.0f, 0};
		rpInfo.clearValueCount = (uint32_t)clearValues.size();
		rpInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Flip viewport Y to match OpenGL convention (Vulkan Y points down)
		VkViewport viewport{};
		viewport.x = 0;
		viewport.y = (float)this->m_swapchainExtent.height;
		viewport.width = (float)this->m_swapchainExtent.width;
		viewport.height = -(float)this->m_swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = this->m_swapchainExtent;
		vkCmdSetScissor(cmd, 0, 1, &scissor);

		VulkanState::setCurrentCommandBuffer(cmd);
	}

	void VulkanContext::endFrame()
	{
		VkCommandBuffer cmd = this->m_commandBuffers[this->m_currentFrame];

		vkCmdEndRenderPass(cmd);
		vkEndCommandBuffer(cmd);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = {this->m_imageAvailableSemaphores[this->m_currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmd;

		VkSemaphore signalSemaphores[] = {this->m_renderFinishedSemaphores[this->m_currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		VkResult submitResult = vkQueueSubmit(this->m_graphicsQueue, 1, &submitInfo, this->m_inFlightFences[this->m_currentFrame]);
		if (submitResult != VK_SUCCESS)
		{
			std::cerr << "[VulkanContext] vkQueueSubmit failed (" << submitResult << ")\n";
			this->m_currentFrame = (this->m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
			return;
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapchains[] = {this->m_swapchain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &this->m_currentImageIndex;

		vkQueuePresentKHR(this->m_presentQueue, &presentInfo);

		this->m_currentFrame = (this->m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	bool VulkanContext::shouldClose()
	{
		return glfwWindowShouldClose(this->m_window);
	}

	void VulkanContext::setClearColor(const Color &color) { this->m_clearColor = color; }
	void VulkanContext::setViewport(int, int, int, int) {} // Handled in beginFrame
	void VulkanContext::setDepthTest(bool enabled) { this->m_depthTestEnabled = enabled; }
	void VulkanContext::setBlending(bool enabled) { this->m_blendingEnabled = enabled; }

	// --- Setup helpers ---

	bool VulkanContext::createInstance()
	{
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "GeniusEngine";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "GeniusEngine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		uint32_t glfwExtCount = 0;
		const char **glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

		if (!glfwExts || glfwExtCount == 0)
		{
			// On macOS, glfwGetRequiredInstanceExtensions can return NULL if
			// the Vulkan loader is available but surface creation isn't supported.
			// Try to provide the extensions manually.
			std::cerr << "[VulkanContext] GLFW didn't provide Vulkan extensions (count=" << glfwExtCount << ").\n"
					  << "  glfwVulkanSupported() = " << glfwVulkanSupported() << "\n"
					  << "  Attempting manual extension list...\n";

#ifdef __APPLE__
			// MoltenVK on macOS needs these
			static const char *macosExts[] = {
				"VK_KHR_surface",
				"VK_EXT_metal_surface",
			};
			glfwExts = macosExts;
			glfwExtCount = 2;
#else
			return false;
#endif
		}

		std::vector<const char *> extensions(glfwExts, glfwExts + glfwExtCount);

#ifdef __APPLE__
		extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
		extensions.push_back("VK_KHR_get_physical_device_properties2");
#endif

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;
		createInfo.enabledExtensionCount = (uint32_t)extensions.size();
		createInfo.ppEnabledExtensionNames = extensions.data();
#ifdef __APPLE__
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

		VkResult result = vkCreateInstance(&createInfo, nullptr, &this->m_instance);
		if (result != VK_SUCCESS)
		{
			std::cerr << "[VulkanContext] Failed to create Vulkan instance (VkResult=" << result << ")\n";
			return false;
		}
		return true;
	}

	bool VulkanContext::pickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(this->m_instance, &deviceCount, nullptr);
		if (deviceCount == 0)
		{
			std::cerr << "[VulkanContext] No Vulkan-capable GPU found\n";
			return false;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(this->m_instance, &deviceCount, devices.data());

		for (auto &dev : devices)
		{
			// Find a device with graphics + present support
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, queueFamilies.data());

			bool foundGraphics = false, foundPresent = false;
			for (uint32_t i = 0; i < queueFamilyCount; i++)
			{
				if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					this->m_graphicsFamily = i;
					foundGraphics = true;
				}
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, this->m_surface, &presentSupport);
				if (presentSupport)
				{
					this->m_presentFamily = i;
					foundPresent = true;
				}
				if (foundGraphics && foundPresent)
					break;
			}

			if (foundGraphics && foundPresent)
			{
				this->m_physicalDevice = dev;
				VkPhysicalDeviceProperties props;
				vkGetPhysicalDeviceProperties(dev, &props);

				this->m_deviceName = props.deviceName;
				this->m_apiVersion = std::to_string(VK_VERSION_MAJOR(props.apiVersion)) + "." +
									 std::to_string(VK_VERSION_MINOR(props.apiVersion)) + "." +
									 std::to_string(VK_VERSION_PATCH(props.apiVersion));
				this->m_driverVersion = std::to_string(VK_VERSION_MAJOR(props.driverVersion)) + "." +
										std::to_string(VK_VERSION_MINOR(props.driverVersion)) + "." +
										std::to_string(VK_VERSION_PATCH(props.driverVersion));

				std::cout << "[VulkanContext] GPU: " << this->m_deviceName
						  << " (Vulkan " << this->m_apiVersion << ")\n";
				return true;
			}
		}

		std::cerr << "[VulkanContext] No suitable GPU found\n";
		return false;
	}

	bool VulkanContext::createLogicalDevice()
	{
		std::set<uint32_t> uniqueFamilies = {this->m_graphicsFamily, this->m_presentFamily};
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		float queuePriority = 1.0f;

		for (uint32_t family : uniqueFamilies)
		{
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = family;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.fillModeNonSolid = VK_TRUE;

		std::vector<const char *> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
			"VK_KHR_portability_subset",
#endif
		};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();

		if (vkCreateDevice(this->m_physicalDevice, &createInfo, nullptr, &this->m_device) != VK_SUCCESS)
		{
			std::cerr << "[VulkanContext] Failed to create logical device\n";
			return false;
		}

		vkGetDeviceQueue(this->m_device, this->m_graphicsFamily, 0, &this->m_graphicsQueue);
		vkGetDeviceQueue(this->m_device, this->m_presentFamily, 0, &this->m_presentQueue);
		return true;
	}

	bool VulkanContext::createSwapchain()
	{
		VkSurfaceCapabilitiesKHR caps;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->m_physicalDevice, this->m_surface, &caps);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(this->m_physicalDevice, this->m_surface, &formatCount, nullptr);
		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(this->m_physicalDevice, this->m_surface, &formatCount, formats.data());

		// Pick format — prefer UNORM to match OpenGL's non-gamma-corrected output.
		// SRGB would double-gamma since vertex colors are already in sRGB.
		this->m_swapchainFormat = formats[0].format;
		VkColorSpaceKHR colorSpace = formats[0].colorSpace;
		for (auto &fmt : formats)
		{
			if (fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				this->m_swapchainFormat = fmt.format;
				colorSpace = fmt.colorSpace;
				break;
			}
		}

		this->m_swapchainExtent = {(uint32_t)this->m_width, (uint32_t)this->m_height};
		if (caps.currentExtent.width != UINT32_MAX)
			this->m_swapchainExtent = caps.currentExtent;

		uint32_t imageCount = caps.minImageCount + 1;
		if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
			imageCount = caps.maxImageCount;

		VkSwapchainCreateInfoKHR swapInfo{};
		swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapInfo.surface = this->m_surface;
		swapInfo.minImageCount = imageCount;
		swapInfo.imageFormat = this->m_swapchainFormat;
		swapInfo.imageColorSpace = colorSpace;
		swapInfo.imageExtent = this->m_swapchainExtent;
		swapInfo.imageArrayLayers = 1;
		swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		uint32_t queueFamilyIndices[] = {this->m_graphicsFamily, this->m_presentFamily};
		if (this->m_graphicsFamily != this->m_presentFamily)
		{
			swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapInfo.queueFamilyIndexCount = 2;
			swapInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		swapInfo.preTransform = caps.currentTransform;
		swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // Vsync
		swapInfo.clipped = VK_TRUE;

		if (vkCreateSwapchainKHR(this->m_device, &swapInfo, nullptr, &this->m_swapchain) != VK_SUCCESS)
		{
			std::cerr << "[VulkanContext] Failed to create swapchain\n";
			return false;
		}

		vkGetSwapchainImagesKHR(this->m_device, this->m_swapchain, &imageCount, nullptr);
		this->m_swapchainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(this->m_device, this->m_swapchain, &imageCount, this->m_swapchainImages.data());

		this->m_swapchainImageViews.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; i++)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = this->m_swapchainImages[i];
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = this->m_swapchainFormat;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(this->m_device, &viewInfo, nullptr, &this->m_swapchainImageViews[i]) != VK_SUCCESS)
			{
				std::cerr << "[VulkanContext] Failed to create image view\n";
				return false;
			}
		}

		return true;
	}

	bool VulkanContext::createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = this->m_swapchainFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = VK_FORMAT_D32_SFLOAT;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorRef{};
		colorRef.attachment = 0;
		colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthRef{};
		depthRef.attachment = 1;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorRef;
		subpass.pDepthStencilAttachment = &depthRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

		VkRenderPassCreateInfo rpInfo{};
		rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rpInfo.attachmentCount = (uint32_t)attachments.size();
		rpInfo.pAttachments = attachments.data();
		rpInfo.subpassCount = 1;
		rpInfo.pSubpasses = &subpass;
		rpInfo.dependencyCount = 1;
		rpInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(this->m_device, &rpInfo, nullptr, &this->m_renderPass) != VK_SUCCESS)
		{
			std::cerr << "[VulkanContext] Failed to create render pass\n";
			return false;
		}
		return true;
	}

	bool VulkanContext::createDepthResources()
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = this->m_swapchainExtent.width;
		imageInfo.extent.height = this->m_swapchainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_D32_SFLOAT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		vkCreateImage(this->m_device, &imageInfo, nullptr, &this->m_depthImage);

		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(this->m_device, this->m_depthImage, &memReqs);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		vkAllocateMemory(this->m_device, &allocInfo, nullptr, &this->m_depthImageMemory);
		vkBindImageMemory(this->m_device, this->m_depthImage, this->m_depthImageMemory, 0);

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = this->m_depthImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = VK_FORMAT_D32_SFLOAT;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(this->m_device, &viewInfo, nullptr, &this->m_depthImageView);
		return true;
	}

	bool VulkanContext::createFramebuffers()
	{
		this->m_framebuffers.resize(this->m_swapchainImageViews.size());
		for (size_t i = 0; i < this->m_swapchainImageViews.size(); i++)
		{
			std::array<VkImageView, 2> attachments = {this->m_swapchainImageViews[i], this->m_depthImageView};

			VkFramebufferCreateInfo fbInfo{};
			fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbInfo.renderPass = this->m_renderPass;
			fbInfo.attachmentCount = (uint32_t)attachments.size();
			fbInfo.pAttachments = attachments.data();
			fbInfo.width = this->m_swapchainExtent.width;
			fbInfo.height = this->m_swapchainExtent.height;
			fbInfo.layers = 1;

			if (vkCreateFramebuffer(this->m_device, &fbInfo, nullptr, &this->m_framebuffers[i]) != VK_SUCCESS)
			{
				std::cerr << "[VulkanContext] Failed to create framebuffer\n";
				return false;
			}
		}
		return true;
	}

	bool VulkanContext::createCommandPool()
	{
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = this->m_graphicsFamily;

		if (vkCreateCommandPool(this->m_device, &poolInfo, nullptr, &this->m_commandPool) != VK_SUCCESS)
		{
			std::cerr << "[VulkanContext] Failed to create command pool\n";
			return false;
		}
		return true;
	}

	bool VulkanContext::createCommandBuffers()
	{
		this->m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = this->m_commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)this->m_commandBuffers.size();

		if (vkAllocateCommandBuffers(this->m_device, &allocInfo, this->m_commandBuffers.data()) != VK_SUCCESS)
		{
			std::cerr << "[VulkanContext] Failed to allocate command buffers\n";
			return false;
		}
		return true;
	}

	bool VulkanContext::createSyncObjects()
	{
		this->m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		this->m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		this->m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semInfo{};
		semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			if (vkCreateSemaphore(this->m_device, &semInfo, nullptr, &this->m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(this->m_device, &semInfo, nullptr, &this->m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(this->m_device, &fenceInfo, nullptr, &this->m_inFlightFences[i]) != VK_SUCCESS)
			{
				std::cerr << "[VulkanContext] Failed to create sync objects\n";
				return false;
			}
		}
		return true;
	}

	void VulkanContext::cleanupSwapchain()
	{
		if (this->m_depthImageView)
			vkDestroyImageView(this->m_device, this->m_depthImageView, nullptr);
		if (this->m_depthImage)
			vkDestroyImage(this->m_device, this->m_depthImage, nullptr);
		if (this->m_depthImageMemory)
			vkFreeMemory(this->m_device, this->m_depthImageMemory, nullptr);

		for (auto fb : this->m_framebuffers)
			vkDestroyFramebuffer(this->m_device, fb, nullptr);
		for (auto iv : this->m_swapchainImageViews)
			vkDestroyImageView(this->m_device, iv, nullptr);

		if (this->m_renderPass)
			vkDestroyRenderPass(this->m_device, this->m_renderPass, nullptr);
		if (this->m_swapchain)
			vkDestroySwapchainKHR(this->m_device, this->m_swapchain, nullptr);
	}

	uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(this->m_physicalDevice, &memProps);

		for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & properties) == properties)
				return i;
		}
		return 0;
	}

} // namespace GE::Graphics::Vulkan
