#include "Input.hpp"
#include <GLFW/glfw3.h>

namespace GE
{

	void Input::init(GLFWwindow *window)
	{
		this->m_window = window;
		glfwSetWindowUserPointer(window, this);
		glfwSetScrollCallback(window, scrollCallback);
	}

	void Input::update()
	{
		if (!this->m_window)
			return;

		// Swap key state
		this->m_keyPrevious = this->m_keyCurrent;

		// Poll all tracked keys + common ones
		static const int trackedKeys[] = {
			32,
			48,
			49,
			50,
			51,
			52,
			53,
			54,
			55,
			56,
			57, // Space, 0-9
			65,
			66,
			67,
			68,
			69,
			70,
			71,
			72,
			73,
			74,
			75,
			76,
			77, // A-M
			78,
			79,
			80,
			81,
			82,
			83,
			84,
			85,
			86,
			87,
			88,
			89,
			90, // N-Z
			256,
			257,
			258,
			259,
			260,
			261, // Escape-Delete
			262,
			263,
			264,
			265, // Arrow keys
			290,
			291,
			292,
			293,
			294,
			295,
			296,
			297,
			298,
			299,
			300,
			301, // F1-F12
			340,
			341,
			342,
			344,
			345,
			346, // Shift/Ctrl/Alt
		};
		for (int k : trackedKeys)
			this->m_keyCurrent[k] = (glfwGetKey(this->m_window, k) == GLFW_PRESS);

		// Mouse buttons
		for (int i = 0; i < 3; i++)
		{
			this->m_mousePrevious[i] = this->m_mouseCurrent[i];
			this->m_mouseCurrent[i] = (glfwGetMouseButton(this->m_window, i) == GLFW_PRESS);
		}

		// Mouse position & delta
		double mx, my;
		glfwGetCursorPos(this->m_window, &mx, &my);
		this->m_mousePos = {(float)mx, (float)my};

		if (this->m_firstFrame)
		{
			this->m_lastMousePos = this->m_mousePos;
			this->m_firstFrame = false;
		}

		this->m_mouseDelta = this->m_mousePos - this->m_lastMousePos;
		this->m_lastMousePos = this->m_mousePos;

		// Scroll (accumulated by callback, consumed here)
		this->m_scrollDelta = s_scrollAccum;
		s_scrollAccum = 0.0f;
	}

	bool Input::isKeyDown(Key key) const
	{
		auto it = this->m_keyCurrent.find((int)key);
		return it != this->m_keyCurrent.end() && it->second;
	}

	bool Input::isKeyPressed(Key key) const
	{
		int k = (int)key;
		auto curr = this->m_keyCurrent.find(k);
		auto prev = this->m_keyPrevious.find(k);
		bool now = (curr != this->m_keyCurrent.end() && curr->second);
		bool was = (prev != this->m_keyPrevious.end() && prev->second);
		return now && !was;
	}

	bool Input::isKeyReleased(Key key) const
	{
		int k = (int)key;
		auto curr = this->m_keyCurrent.find(k);
		auto prev = this->m_keyPrevious.find(k);
		bool now = (curr != this->m_keyCurrent.end() && curr->second);
		bool was = (prev != this->m_keyPrevious.end() && prev->second);
		return !now && was;
	}

	bool Input::isMouseDown(MouseButton btn) const
	{
		return this->m_mouseCurrent[(int)btn];
	}

	bool Input::isMousePressed(MouseButton btn) const
	{
		int i = (int)btn;
		return this->m_mouseCurrent[i] && !this->m_mousePrevious[i];
	}

	bool Input::isMouseReleased(MouseButton btn) const
	{
		int i = (int)btn;
		return !this->m_mouseCurrent[i] && this->m_mousePrevious[i];
	}

	void Input::requestClose()
	{
		if (this->m_window)
			glfwSetWindowShouldClose(this->m_window, GLFW_TRUE);
	}

	void Input::scrollCallback(GLFWwindow *, double, double yoffset)
	{
		s_scrollAccum += (float)yoffset;
	}

} // namespace GE
