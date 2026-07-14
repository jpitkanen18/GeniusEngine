#pragma once
// GeniusEngine - UI abstraction (wraps ImGui, backend-agnostic interface)

#include "../core/Types.hpp"
#include "../graphics/GAPI.hpp"
#include <functional>

#ifdef GE_HAS_VULKAN
#include <vulkan/vulkan.h>
#endif

namespace GE::UI
{

	// Abstract widget types so the app doesn't directly depend on ImGui
	struct PanelConfig
	{
		std::string title;
		Vec2 position{0, 0};
		Vec2 size{300, 400};
		bool movable = true;
		bool resizable = true;
		float alpha = 0.95f;
	};

	class UI
	{
	public:
		UI() = default;
		~UI();

		bool init(void *nativeWindow, Graphics::Backend backend = Graphics::Backend::OpenGL);
		void shutdown();

		void beginFrame();
		void endFrame();

		// High-level panel/widget API
		bool beginPanel(const std::string &title, bool *open = nullptr);
		void endPanel();

		// Widgets
		void text(const std::string &str);
		void textColored(const Color &color, const std::string &str);
		bool button(const std::string &label, const Vec2 &size = {0, 0});
		bool checkbox(const std::string &label, bool &value);
		bool sliderFloat(const std::string &label, float &value, float min, float max);
		bool sliderFloat3(const std::string &label, Vec3 &value, float min, float max);
		bool colorEdit3(const std::string &label, Vec3 &color);
		bool inputFloat(const std::string &label, float &value, float step = 0.1f);
		void separator();
		void spacing();
		bool treeNode(const std::string &label);
		void treePop();
		void sameLine(float offset = 0.0f);
		void progressBar(float fraction, const std::string &overlay = "");

		// Combo/dropdown
		bool beginCombo(const std::string &label, const std::string &preview);
		bool selectableItem(const std::string &label, bool selected);
		void endCombo();

	private:
		bool m_initialized = false;
		Graphics::Backend m_backend = Graphics::Backend::OpenGL;
#ifdef GE_HAS_VULKAN
		VkDescriptorPool m_vkDescriptorPool = VK_NULL_HANDLE;
#endif
	};

} // namespace GE::UI
