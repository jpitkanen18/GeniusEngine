#pragma once
// GeniusEngine - Window abstraction (backend-agnostic)

#include "../core/Types.hpp"
#include "../graphics/GAPI.hpp"
#include <functional>

namespace GE
{

	struct WindowConfig
	{
		int width = 1280;
		int height = 720;
		std::string title = "GeniusEngine";
		bool vsync = true;
		bool resizable = true;
		Graphics::Backend backend = Graphics::Backend::OpenGL;
	};

	class Window
	{
	public:
		Window() = default;
		~Window();

		bool create(const WindowConfig &config);
		void destroy();

		bool shouldClose() const;
		void beginFrame();
		void endFrame();

		int getWidth() const { return this->m_config.width; }
		int getHeight() const { return this->m_config.height; }
		float getAspect() const { return (float)this->m_config.width / (float)this->m_config.height; }
		void *getNativeWindow() const;
		Graphics::IContext *getContext() { return this->m_context.get(); }

		using ResizeCallback = std::function<void(int, int)>;
		void setResizeCallback(ResizeCallback cb) { this->m_resizeCb = cb; }

	private:
		WindowConfig m_config;
		std::unique_ptr<Graphics::IContext> m_context;
		ResizeCallback m_resizeCb;
	};

} // namespace GE
