#include "Window.hpp"
#include "../graphics/opengl/GLContext.hpp"
#ifdef __APPLE__
#include "../graphics/metal/MetalContext.hpp"
#endif
#ifdef GE_HAS_VULKAN
#include "../graphics/vulkan/VulkanContext.hpp"
#endif

namespace GE
{

	Window::~Window() { destroy(); }

	bool Window::create(const WindowConfig &config)
	{
		this->m_config = config;

		std::unique_ptr<Graphics::IContext> ctx;
		switch (config.backend)
		{
#ifdef __APPLE__
		case Graphics::Backend::Metal:
			ctx = std::make_unique<Graphics::Metal::MetalContext>();
			break;
#else
		case Graphics::Backend::Metal:
			std::cerr << "[GeniusEngine] Metal backend is only available on macOS.\n";
			return false;
#endif

#ifdef GE_HAS_VULKAN
		case Graphics::Backend::Vulkan:
			ctx = std::make_unique<Graphics::Vulkan::VulkanContext>();
			break;
#else
		case Graphics::Backend::Vulkan:
			std::cerr << "[GeniusEngine] Vulkan backend was not found during configuration.\n"
					  << "  Install the Vulkan SDK and re-run ./configure.sh\n";
			return false;
#endif

		case Graphics::Backend::OpenGL:
		default:
			ctx = std::make_unique<Graphics::GL::GLContext>();
			break;
		}

		if (!ctx->init(config.width, config.height, config.title))
		{
			return false;
		}
		ctx->setClearColor({0.02f, 0.02f, 0.05f, 1.0f});
		this->m_context = std::move(ctx);
		return true;
	}

	void Window::destroy()
	{
		if (this->m_context)
		{
			this->m_context->shutdown();
			this->m_context.reset();
		}
	}

	bool Window::shouldClose() const
	{
		return this->m_context ? this->m_context->shouldClose() : true;
	}

	void Window::beginFrame()
	{
		if (this->m_context)
			this->m_context->beginFrame();
	}

	void Window::endFrame()
	{
		if (this->m_context)
			this->m_context->endFrame();
	}

	void *Window::getNativeWindow() const
	{
		return this->m_context ? this->m_context->getNativeWindow() : nullptr;
	}

} // namespace GE
