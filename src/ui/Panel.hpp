#pragma once
// GeniusEngine - Declarative UI Panel
// Subclass this to define a reusable panel layout. The engine renders it
// automatically — you just update the bound data and call refresh methods.
//
// Usage:
//   class MyPanel : public GE::UI::Panel {
//   public:
//       float speed = 1.0f;
//       bool paused = false;
//
//       void layout() override {
//           slider("Speed", speed, 0.0f, 10.0f);
//           checkbox("Paused", paused);
//           separator();
//           label("FPS: " + std::to_string(fps));
//       }
//   };
//
// Then in your Application:
//   MyPanel panel{"Controls"};
//   ...
//   void onUI() override { panel.draw(ui()); }

#include "../core/Types.hpp"
#include <string>
#include <vector>
#include <functional>

namespace GE::UI
{

	class UI; // forward

	class Panel
	{
	public:
		Panel() = default;
		explicit Panel(const std::string &title) : m_title(title) {}
		virtual ~Panel() = default;

		// Set the panel title
		void setTitle(const std::string &title) { this->m_title = title; }
		const std::string &getTitle() const { return this->m_title; }

		// Show/hide the panel
		void setVisible(bool visible) { this->m_visible = visible; }
		bool isVisible() const { return this->m_visible; }

		// Call from onUI() to render this panel
		void draw(UI &ctx);

	protected:
		// Override this to define your panel layout.
		// Use the widget methods below — they record and render in one pass.
		virtual void layout() = 0;

		// --- Widget methods (call inside layout()) ---

		void label(const std::string &text);
		void labelColored(const Color &color, const std::string &text);
		void separator();
		void spacing();
		void sameLine(float offset = 0.0f);

		bool button(const std::string &label, const Vec2 &size = {0, 0});
		bool checkbox(const std::string &label, bool &value);
		bool slider(const std::string &label, float &value, float min, float max);
		bool slider3(const std::string &label, Vec3 &value, float min, float max);
		bool colorEdit(const std::string &label, Vec3 &color);
		bool inputFloat(const std::string &label, float &value, float step = 0.1f);

		bool beginCombo(const std::string &label, const std::string &preview);
		bool selectableItem(const std::string &label, bool selected);
		void endCombo();

		bool treeNode(const std::string &label);
		void treePop();

		void progressBar(float fraction, const std::string &overlay = "");

		// Convenience: run a block conditionally
		// Usage: section("Advanced", [&]{ slider("X", x, 0, 1); });
		void section(const std::string &label, std::function<void()> body);

	private:
		std::string m_title = "Panel";
		bool m_visible = true;
		UI *m_ctx = nullptr; // set during draw()
	};

} // namespace GE::UI
