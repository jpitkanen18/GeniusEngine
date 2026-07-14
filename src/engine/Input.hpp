#pragma once
// GeniusEngine - Input abstraction
// Provides backend-agnostic key/mouse/scroll queries.
// Updated each frame by the engine — apps never touch GLFW directly.

#include "../core/Types.hpp"
#include <unordered_map>

struct GLFWwindow;

namespace GE
{

	// Keyboard key codes (mirrors GLFW but decoupled from it)
	enum class Key
	{
		// Letters
		A = 65,
		B,
		C,
		D,
		E,
		F,
		G,
		H,
		I,
		J,
		K,
		L,
		M,
		N,
		O,
		P,
		Q,
		R,
		S,
		T,
		U,
		V,
		W,
		X,
		Y,
		Z,
		// Numbers
		Num0 = 48,
		Num1,
		Num2,
		Num3,
		Num4,
		Num5,
		Num6,
		Num7,
		Num8,
		Num9,
		// Function keys
		F1 = 290,
		F2,
		F3,
		F4,
		F5,
		F6,
		F7,
		F8,
		F9,
		F10,
		F11,
		F12,
		// Special
		Space = 32,
		Escape = 256,
		Enter = 257,
		Tab = 258,
		Backspace = 259,
		Insert = 260,
		Delete = 261,
		Right = 262,
		Left = 263,
		Down = 264,
		Up = 265,
		LeftShift = 340,
		LeftControl = 341,
		LeftAlt = 342,
		RightShift = 344,
		RightControl = 345,
		RightAlt = 346,
	};

	enum class MouseButton
	{
		Left = 0,
		Right = 1,
		Middle = 2,
	};

	class Input
	{
	public:
		Input() = default;

		void init(GLFWwindow *window);
		void update(); // Called once per frame by the engine

		// --- Keyboard ---

		// True while the key is held down
		bool isKeyDown(Key key) const;

		// True only on the frame the key was first pressed
		bool isKeyPressed(Key key) const;

		// True only on the frame the key was released
		bool isKeyReleased(Key key) const;

		// --- Mouse buttons ---

		bool isMouseDown(MouseButton btn) const;
		bool isMousePressed(MouseButton btn) const;
		bool isMouseReleased(MouseButton btn) const;

		// --- Mouse position & delta ---

		Vec2 getMousePosition() const { return this->m_mousePos; }
		Vec2 getMouseDelta() const { return this->m_mouseDelta; }

		// --- Scroll ---

		float getScrollDelta() const { return this->m_scrollDelta; }

		// --- Window control ---

		void requestClose();

	private:
		GLFWwindow *m_window = nullptr;

		// Current and previous frame key state
		std::unordered_map<int, bool> m_keyCurrent;
		std::unordered_map<int, bool> m_keyPrevious;

		// Current and previous frame mouse button state
		bool m_mouseCurrent[3] = {};
		bool m_mousePrevious[3] = {};

		Vec2 m_mousePos{0.0f};
		Vec2 m_lastMousePos{0.0f};
		Vec2 m_mouseDelta{0.0f};
		bool m_firstFrame = true;

		float m_scrollDelta = 0.0f;

		// Scroll callback writes here
		static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);
		static inline float s_scrollAccum = 0.0f;
	};

} // namespace GE
