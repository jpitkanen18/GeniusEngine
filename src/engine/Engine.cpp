#include "Engine.hpp"
#include "../graphics/Factory.hpp"
#include <GLFW/glfw3.h>

namespace GE
{

	bool Engine::init(const EngineConfig &config)
	{
		// Set the factory backend before creating anything
		Graphics::Factory::setBackend(config.window.backend);

		if (!this->m_window.create(config.window))
		{
			std::cerr << "[Engine] Failed to create window\n";
			return false;
		}

		this->m_renderer.init();
		this->m_camera.setPerspective(60.0f, this->m_window.getAspect(), 0.1f, 10000.0f);
		this->m_camera.lookAt({0, 30, 80}, {0, 0, 0}, {0, 1, 0});

		this->m_input.init(static_cast<GLFWwindow *>(this->m_window.getNativeWindow()));

		this->m_lastFrameTime = glfwGetTime();
		return true;
	}

	void Engine::shutdown()
	{
		this->m_window.destroy();
	}

	void Engine::beginFrame()
	{
		double currentTime = glfwGetTime();
		this->m_deltaTime = static_cast<float>(currentTime - this->m_lastFrameTime);
		this->m_lastFrameTime = currentTime;
		this->m_time += this->m_deltaTime;
		this->m_timeDouble = currentTime;

		this->m_window.beginFrame();
		this->m_input.update();
		this->m_renderer.beginScene(this->m_camera);
	}

	void Engine::endFrame()
	{
		this->m_renderer.endScene();
		this->m_window.endFrame();
	}

	bool Engine::isRunning() const
	{
		return !this->m_window.shouldClose();
	}

} // namespace GE
