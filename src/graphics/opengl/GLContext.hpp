#pragma once
// OpenGL Context Implementation

#include "../GAPI.hpp"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

namespace GE::Graphics::GL
{

	class GLContext : public IContext
	{
	public:
		GLContext() = default;
		~GLContext() override { shutdown(); }

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
		std::string m_deviceName;
		std::string m_driverVersion;
		std::string m_apiVersion;
	};

} // namespace GE::Graphics::GL
