#include "GLContext.hpp"

namespace GE::Graphics::GL
{

	bool GLContext::init(int width, int height, const std::string &title)
	{
		if (!glfwInit())
		{
			std::cerr << "[GLContext] Failed to initialize GLFW\n";
			return false;
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

		this->m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		if (!this->m_window)
		{
			std::cerr << "[GLContext] Failed to create window\n";
			glfwTerminate();
			return false;
		}

		glfwMakeContextCurrent(this->m_window);

		if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
		{
			std::cerr << "[GLContext] Failed to initialize GLAD\n";
			return false;
		}

		this->m_width = width;
		this->m_height = height;

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		std::cout << "[GLContext] OpenGL " << glGetString(GL_VERSION) << "\n";

		this->m_deviceName = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
		this->m_apiVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));
		this->m_driverVersion = reinterpret_cast<const char *>(glGetString(GL_VENDOR));

		return true;
	}

	void GLContext::shutdown()
	{
		if (this->m_window)
		{
			glfwDestroyWindow(this->m_window);
			this->m_window = nullptr;
			glfwTerminate();
		}
	}

	void GLContext::beginFrame()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void GLContext::endFrame()
	{
		glfwSwapBuffers(this->m_window);
		glfwPollEvents();
	}

	bool GLContext::shouldClose()
	{
		return glfwWindowShouldClose(this->m_window);
	}

	void GLContext::setClearColor(const Color &color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void GLContext::setViewport(int x, int y, int w, int h)
	{
		glViewport(x, y, w, h);
		this->m_width = w;
		this->m_height = h;
	}

	void GLContext::setDepthTest(bool enabled)
	{
		if (enabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}

	void GLContext::setBlending(bool enabled)
	{
		if (enabled)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}

} // namespace GE::Graphics::GL
