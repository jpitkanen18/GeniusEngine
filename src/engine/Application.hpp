#pragma once
// GeniusEngine - Application base class
// Subclass this and override lifecycle methods to build your game.
// The engine owns main() — your game file is just the class.
//
// Usage (entire game.cpp):
//
//   #include <engine/Application.hpp>
//
//   class MyGame : public GE::Application {
//       void configure(EngineConfig& config) override { ... }
//       void onInit() override { ... }
//       void onUpdate(float dt) override { ... }
//       void onRender() override { ... }
//       void onUI() override { ... }
//   };
//
//   GE_APP(MyGame)

#include "Engine.hpp"
#include "../ui/UI.hpp"

namespace GE
{

	class Application
	{
	public:
		Application() = default;
		virtual ~Application() = default;

		// --- Lifecycle (override these) ---

		// Called before engine init. Set window size, title, backend, etc.
		virtual void configure(EngineConfig &config) { (void)config; }

		// Called once after engine + UI are initialized. Load assets here.
		virtual void onInit() {}

		// Called once before shutdown. Free resources here.
		virtual void onShutdown() {}

		// Called every frame for input handling.
		virtual void onInput(float dt) { (void)dt; }

		// Called every frame for simulation/logic.
		virtual void onUpdate(float dt) { (void)dt; }

		// Called every frame between beginFrame/endFrame for rendering.
		virtual void onRender() {}

		// Called every frame for UI drawing (between ui begin/end frame).
		virtual void onUI() {}

		// Return false to hide UI this frame (still called, just not rendered)
		virtual bool wantsUI() { return true; }

		// --- Engine access ---

		Engine &engine() { return this->m_engine; }
		Renderer &renderer() { return this->m_engine.getRenderer(); }
		Camera &camera() { return this->m_engine.getCamera(); }
		Window &window() { return this->m_engine.getWindow(); }
		Input &input() { return this->m_engine.getInput(); }
		UI::UI &ui() { return this->m_ui; }
		float deltaTime() const { return this->m_engine.getDeltaTime(); }
		float time() const { return this->m_engine.getTime(); }

		// --- Run loop (called internally by entry point) ---
		int run()
		{
			EngineConfig config;
			configure(config);

			if (!this->m_engine.init(config))
			{
				std::cerr << "[GeniusEngine] Failed to initialize engine\n";
				return 1;
			}

			this->m_ui.init(this->m_engine.getWindow().getNativeWindow(), config.window.backend);
			onInit();

			while (this->m_engine.isRunning())
			{
				this->m_engine.beginFrame();
				float dt = this->m_engine.getDeltaTime();
				onInput(dt);
				onUpdate(dt);
				onRender();

				if (wantsUI())
				{
					this->m_ui.beginFrame();
					onUI();
					this->m_ui.endFrame();
				}

				this->m_engine.endFrame();
			}

			onShutdown();
			this->m_ui.shutdown();
			this->m_engine.shutdown();
			return 0;
		}

	private:
		Engine m_engine;
		UI::UI m_ui;
	};

} // namespace GE

// Register your Application subclass. Place at the bottom of your game file.
// The engine provides the run loop — this just wires up the entry point.
#define GE_APP(AppClass)  \
	int main()            \
	{                     \
		AppClass app;     \
		return app.run(); \
	}
