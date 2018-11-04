#pragma once

#include <GL/glew.h>

#include "Engine.h"
#include "Input.h"

class Window
{
public:
	Window(int width, int height, const std::string& title);
	~Window();

	static Window* getInstance();

	void setTitle(const std::string& title);
	
	void resize(int width, int height);
	
	void close();

	void mainLoop();

private:
	static Window* m_instance;

	Input* m_input;
	Engine* m_engine;

	GLFWwindow* m_window;

	int m_width;
	int m_height;

	std::string m_title;

	double m_prevTime;

	float m_fpsAccumulator;
	int m_fpsFrameCount;
};

void glMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam);
void glfwErrorCallback(int error, const char* description);
void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void glfwFrameBufferResizeCallback(GLFWwindow* window, int width, int height);
void glfwMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);