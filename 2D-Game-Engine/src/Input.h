#pragma once

#include <GLFW/glfw3.h>
#include <glm.hpp>

class Input
{
public:
	Input();
	~Input();

	static Input* getInstance();

	void update();

	bool isKeyPressed(int key) const;
	bool isKeyHeld(int key) const;
	bool isKeyReleased(int key) const;

	bool isMouseButtonPressed(int button) const;
	bool isMouseButtonHeld(int button) const;
	bool isMouseButtonReleased(int button) const;

	glm::vec2 getMousePosition() const;

	void _pressKey(int key);
	void _releaseKey(int key);
	void _setMousePosition(float x, float y);
	void _pressMouseButton(int button);
	void _releaseMouseButton(int button);

private:
	static Input* m_instance;

	bool* m_keyboardState;
	bool* m_prevKeyboardState;
	bool* m_mouseState;
	bool* m_prevMouseState;
	glm::vec2 m_mousePosition;
};