#include "stdafx.h"
#include "Input.h"

Input* Input::m_instance = nullptr;

Input::Input()
{
	if (!m_instance)
		m_instance = this;
	else
	{
		Output::error("Attempted to create a second input instance - this is not supported. Use Input::getInstance() instead.");
		exit(EXIT_FAILURE);
	}

	m_keyboardState = new bool[GLFW_KEY_LAST + 1];
	m_prevKeyboardState = new bool[GLFW_KEY_LAST + 1];
	m_mouseState = new bool[GLFW_MOUSE_BUTTON_LAST + 1];
	m_prevMouseState = new bool[GLFW_MOUSE_BUTTON_LAST + 1];

	memset(m_keyboardState, 0, GLFW_KEY_LAST + 1);
	memset(m_prevKeyboardState, 0, GLFW_KEY_LAST + 1);
	memset(m_mouseState, 0, GLFW_MOUSE_BUTTON_LAST + 1);
	memset(m_prevMouseState, 0, GLFW_MOUSE_BUTTON_LAST + 1);
}

Input::~Input()
{
	delete[] m_keyboardState;
	delete[] m_prevKeyboardState;
	delete[] m_mouseState;
	delete[] m_prevMouseState;

	m_instance = nullptr;
}

Input* Input::getInstance()
{
	return m_instance;
}

void Input::update()
{
	memcpy_s(m_prevKeyboardState, GLFW_KEY_LAST + 1, m_keyboardState, GLFW_KEY_LAST + 1);
	memcpy_s(m_prevMouseState, GLFW_MOUSE_BUTTON_LAST + 1, m_mouseState, GLFW_MOUSE_BUTTON_LAST + 1);
}

bool Input::isKeyPressed(int key) const
{
	assert(key > 0 && key < GLFW_KEY_LAST + 1);
	return m_keyboardState[key] && !m_prevKeyboardState[key];
}

bool Input::isKeyHeld(int key) const
{
	assert(key > 0 && key < GLFW_KEY_LAST + 1);
	return m_keyboardState[key] && m_prevKeyboardState[key];
}

bool Input::isKeyReleased(int key) const
{
	assert(key > 0 && key < GLFW_KEY_LAST + 1);
	return !m_keyboardState[key] && m_prevKeyboardState[key];
}

bool Input::isMouseButtonPressed(int button) const
{
	assert(button > 0 && button < GLFW_MOUSE_BUTTON_LAST + 1);
	return m_mouseState[button] && !m_prevMouseState[button];
}

bool Input::isMouseButtonHeld(int button) const
{
	assert(button > 0 && button < GLFW_MOUSE_BUTTON_LAST + 1);
	return m_mouseState[button] && m_prevMouseState[button];
}

bool Input::isMouseButtonReleased(int button) const
{
	assert(button > 0 && button < GLFW_MOUSE_BUTTON_LAST + 1);
	return !m_mouseState[button] && m_prevMouseState[button];
}

glm::vec2 Input::getMousePosition() const
{
	return m_mousePosition;
}

void Input::_pressKey(int key)
{
	assert(key > 0 && key < GLFW_KEY_LAST + 1);
	m_keyboardState[key] = true;
}

void Input::_releaseKey(int key)
{
	assert(key > 0 && key < GLFW_KEY_LAST + 1);
	m_keyboardState[key] = false;
}

void Input::_setMousePosition(float x, float y)
{
	m_mousePosition = glm::vec2(x, y);
}

void Input::_pressMouseButton(int button)
{
	assert(button > 0 && button < GLFW_MOUSE_BUTTON_LAST + 1);
	m_mouseState[button] = true;
}

void Input::_releaseMouseButton(int button)
{
	assert(button > 0 && button < GLFW_MOUSE_BUTTON_LAST + 1);
	m_mouseState[button] = false;
}
