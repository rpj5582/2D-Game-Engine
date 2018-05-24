#include "stdafx.h"
#include "Window.h"

Window* Window::m_instance = nullptr;

void glMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if (type == GL_DEBUG_TYPE_ERROR)
		Output::error("GL ERROR: " + std::string(message));
	else
		Output::log("GL Message: " + std::string(message));
}

void glfwErrorCallback(int error, const char* description)
{
	Output::log("GLFW Error " + std::to_string(error) + ": " + std::string(description));
}

void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
		Input::getInstance()->_pressKey(key);
	else if(action == GLFW_RELEASE)
		Input::getInstance()->_releaseKey(key);
}

void glfwFrameBufferResizeCallback(GLFWwindow* window, int width, int height)
{
	Window::getInstance()->resize(width, height);
}

void glfwMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	Input::getInstance()->_setMousePosition((float)xpos, (float)ypos);
}

void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
}

Window::Window(int width, int height, const std::string& title)
{
	if (!m_instance)
		m_instance = this;
	else
	{
		Output::error("Attempted to create a second window instance - this is not supported. Use Window::getInstance() instead.");
		exit(EXIT_FAILURE);
	}

	m_width = width;
	m_height = height;	

	m_title = title;

	if (!glfwInit())
	{
		Output::error("Failed to initialize GLFW");
		exit(EXIT_FAILURE);
	}

	glfwSetErrorCallback(&glfwErrorCallback);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
	if (!m_window)
	{
		Output::error("Failed to create the window");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(m_window);

	unsigned int error = glewInit();
	if (error)
	{
		const char* errorMessage = (const char*)glewGetErrorString(error);
		Output::error("Failed to initialize GLEW: " + std::string(errorMessage));
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(m_window, &glfwKeyCallback);
	glfwSetCursorPosCallback(m_window, &glfwMousePositionCallback);
	glfwSetFramebufferSizeCallback(m_window, &glfwFrameBufferResizeCallback);

	// Vsync - 1 for on 0 for off
	glfwSwapInterval(1);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(&glMessageCallback, nullptr);

	m_prevTime = glfwGetTime();

	m_fpsAccumulator = 0;
	m_fpsFrameCount = 0;

	m_input = new Input();
	m_engine = new Engine(m_width, m_height);
}

Window::~Window()
{
	m_instance = nullptr;
	delete m_engine;
	delete m_input;

	glfwDestroyWindow(m_window);
	glfwTerminate();
}

Window* Window::getInstance()
{
	return m_instance;
}

void Window::setTitle(const std::string& title)
{
	m_title = title;

#if defined(_DEBUG)
	glfwSetWindowTitle(m_window, (m_title + " - FPS: " + std::to_string(m_fpsFrameCount)).c_str());
#else
	glfwSetWindowTitle(m_window, (m_title);
#endif
}

void Window::resize(int width, int height)
{
	m_width = width;
	m_height = height;

	glViewport(0, 0, width, height);

	m_engine->onWindowResize(m_width, m_height);
}

void Window::close()
{
	glfwSetWindowShouldClose(m_window, GLFW_TRUE);
}

void Window::mainLoop()
{
	while (!glfwWindowShouldClose(m_window))
	{
		double time = glfwGetTime();
		float deltaTime = (float)(time - m_prevTime);

		m_fpsFrameCount++;
		m_fpsAccumulator += deltaTime;
		if (m_fpsAccumulator >= 1.0f)
		{
#if defined(_DEBUG)
			setTitle(m_title);
#endif
			m_fpsAccumulator--;
			m_fpsFrameCount = 0;
		}

		m_engine->update(deltaTime);
		m_input->update();

		glfwSwapBuffers(m_window);
		glfwPollEvents();

		m_prevTime = time;
	}
}
