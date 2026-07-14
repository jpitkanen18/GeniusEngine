#include "UI.hpp"
#include "../graphics/Factory.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include "../graphics/metal/MetalState.hpp"
#import <imgui_impl_metal.h>
#import <Metal/Metal.h>
#endif

#ifdef GE_HAS_VULKAN
#include <imgui_impl_vulkan.h>
#include "../graphics/vulkan/VulkanState.hpp"
#endif

namespace GE::UI
{

	UI::~UI() { shutdown(); }

	bool UI::init(void *nativeWindow, Graphics::Backend backend)
	{
		this->m_backend = backend;

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO &io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

		// Dark theme with custom accent
		ImGui::StyleColorsDark();
		ImGuiStyle &style = ImGui::GetStyle();
		style.WindowRounding = 6.0f;
		style.FrameRounding = 4.0f;
		style.GrabRounding = 4.0f;
		style.WindowBorderSize = 1.0f;

		auto *window = static_cast<GLFWwindow *>(nativeWindow);

		switch (this->m_backend)
		{
#ifdef __APPLE__
		case Graphics::Backend::Metal:
		{
			ImGui_ImplGlfw_InitForOther(window, true);
			id<MTLDevice> device = (__bridge id<MTLDevice>)Graphics::Metal::MetalState::getDevice();
			ImGui_ImplMetal_Init(device);
			break;
		}
#endif
#ifdef GE_HAS_VULKAN
		case Graphics::Backend::Vulkan:
		{
			ImGui_ImplGlfw_InitForVulkan(window, true);

			// Create descriptor pool for ImGui
			VkDescriptorPoolSize poolSizes[] = {
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
			};
			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			poolInfo.maxSets = 100;
			poolInfo.poolSizeCount = 1;
			poolInfo.pPoolSizes = poolSizes;
			vkCreateDescriptorPool(Graphics::Vulkan::VulkanState::getDevice(), &poolInfo, nullptr, &this->m_vkDescriptorPool);

			ImGui_ImplVulkan_InitInfo initInfo{};
			initInfo.Instance = Graphics::Vulkan::VulkanState::getInstance();
			initInfo.PhysicalDevice = Graphics::Vulkan::VulkanState::getPhysicalDevice();
			initInfo.Device = Graphics::Vulkan::VulkanState::getDevice();
			initInfo.QueueFamily = Graphics::Vulkan::VulkanState::getGraphicsQueueFamily();
			initInfo.Queue = Graphics::Vulkan::VulkanState::getGraphicsQueue();
			initInfo.DescriptorPool = this->m_vkDescriptorPool;
			initInfo.MinImageCount = 2;
			initInfo.ImageCount = 2;
			initInfo.RenderPass = Graphics::Vulkan::VulkanState::getRenderPass();
			ImGui_ImplVulkan_Init(&initInfo);
			break;
		}
#endif
		case Graphics::Backend::OpenGL:
		default:
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init("#version 410");
			break;
		}

		this->m_initialized = true;
		return true;
	}

	void UI::shutdown()
	{
		if (this->m_initialized)
		{
			switch (this->m_backend)
			{
#ifdef __APPLE__
			case Graphics::Backend::Metal:
				ImGui_ImplMetal_Shutdown();
				break;
#endif
#ifdef GE_HAS_VULKAN
			case Graphics::Backend::Vulkan:
				ImGui_ImplVulkan_Shutdown();
				if (this->m_vkDescriptorPool)
					vkDestroyDescriptorPool(Graphics::Vulkan::VulkanState::getDevice(), this->m_vkDescriptorPool, nullptr);
				break;
#endif
			case Graphics::Backend::OpenGL:
			default:
				ImGui_ImplOpenGL3_Shutdown();
				break;
			}
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			this->m_initialized = false;
		}
	}

	void UI::beginFrame()
	{
		switch (this->m_backend)
		{
#ifdef __APPLE__
		case Graphics::Backend::Metal:
		{
			MTLRenderPassDescriptor* rpd =
				(__bridge MTLRenderPassDescriptor*)Graphics::Metal::MetalState::getRenderPassDescriptor();
			ImGui_ImplMetal_NewFrame(rpd);
			break;
		}
#endif
#ifdef GE_HAS_VULKAN
		case Graphics::Backend::Vulkan:
			ImGui_ImplVulkan_NewFrame();
			break;
#endif
		case Graphics::Backend::OpenGL:
		default:
			ImGui_ImplOpenGL3_NewFrame();
			break;
		}
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void UI::endFrame()
	{
		ImGui::Render();
		switch (this->m_backend)
		{
#ifdef __APPLE__
		case Graphics::Backend::Metal:
		{
			id<MTLCommandBuffer> cmdBuf =
				(__bridge id<MTLCommandBuffer>)Graphics::Metal::MetalState::getCommandBuffer();
			id<MTLRenderCommandEncoder> encoder =
				(__bridge id<MTLRenderCommandEncoder>)Graphics::Metal::MetalState::getEncoder();
			ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmdBuf, encoder);
			break;
		}
#endif
#ifdef GE_HAS_VULKAN
		case Graphics::Backend::Vulkan:
		{
			VkCommandBuffer cmd = Graphics::Vulkan::VulkanState::getCurrentCommandBuffer();
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
			break;
		}
#endif
		case Graphics::Backend::OpenGL:
		default:
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			break;
		}
	}

	bool UI::beginPanel(const std::string &title, bool *open)
	{
		return ImGui::Begin(title.c_str(), open);
	}

	void UI::endPanel()
	{
		ImGui::End();
	}

	void UI::text(const std::string &str)
	{
		ImGui::TextUnformatted(str.c_str());
	}

	void UI::textColored(const Color &color, const std::string &str)
	{
		ImGui::TextColored(ImVec4(color.r, color.g, color.b, color.a), "%s", str.c_str());
	}

	bool UI::button(const std::string &label, const Vec2 &size)
	{
		return ImGui::Button(label.c_str(), ImVec2(size.x, size.y));
	}

	bool UI::checkbox(const std::string &label, bool &value)
	{
		return ImGui::Checkbox(label.c_str(), &value);
	}

	bool UI::sliderFloat(const std::string &label, float &value, float min, float max)
	{
		return ImGui::SliderFloat(label.c_str(), &value, min, max);
	}

	bool UI::sliderFloat3(const std::string &label, Vec3 &value, float min, float max)
	{
		return ImGui::SliderFloat3(label.c_str(), &value[0], min, max);
	}

	bool UI::colorEdit3(const std::string &label, Vec3 &color)
	{
		return ImGui::ColorEdit3(label.c_str(), &color[0]);
	}

	bool UI::inputFloat(const std::string &label, float &value, float step)
	{
		return ImGui::InputFloat(label.c_str(), &value, step);
	}

	void UI::separator() { ImGui::Separator(); }
	void UI::spacing() { ImGui::Spacing(); }

	bool UI::treeNode(const std::string &label)
	{
		return ImGui::TreeNode(label.c_str());
	}

	void UI::treePop() { ImGui::TreePop(); }

	void UI::sameLine(float offset)
	{
		ImGui::SameLine(offset);
	}

	void UI::progressBar(float fraction, const std::string &overlay)
	{
		ImGui::ProgressBar(fraction, ImVec2(-1, 0), overlay.empty() ? nullptr : overlay.c_str());
	}

	bool UI::beginCombo(const std::string &label, const std::string &preview)
	{
		return ImGui::BeginCombo(label.c_str(), preview.c_str());
	}

	bool UI::selectableItem(const std::string &label, bool selected)
	{
		return ImGui::Selectable(label.c_str(), selected);
	}

	void UI::endCombo()
	{
		ImGui::EndCombo();
	}

} // namespace GE::UI
