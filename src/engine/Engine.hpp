#pragma once
// GeniusEngine - Main engine class (ties everything together)

#include "Window.hpp"
#include "Camera.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "Input.hpp"

namespace GE
{

	struct EngineConfig
	{
		WindowConfig window;
	};

	class Engine
	{
	public:
		Engine() = default;
		~Engine() = default;

		bool init(const EngineConfig &config);
		void shutdown();

		Window &getWindow() { return this->m_window; }
		Renderer &getRenderer() { return this->m_renderer; }
		Camera &getCamera() { return this->m_camera; }
		Scene &getScene() { return this->m_scene; }
		Input &getInput() { return this->m_input; }

		// Frame timing
		float getDeltaTime() const { return this->m_deltaTime; }
		float getTime() const { return this->m_time; }
		double getTimeDouble() const { return this->m_timeDouble; }

		void beginFrame();
		void endFrame();
		bool isRunning() const;

	private:
		Window m_window;
		Renderer m_renderer;
		Camera m_camera;
		Scene m_scene;
		Input m_input;

		float m_deltaTime = 0.0f;
		float m_time = 0.0f;
		double m_timeDouble = 0.0;
		double m_lastFrameTime = 0.0;
	};

} // namespace GE
