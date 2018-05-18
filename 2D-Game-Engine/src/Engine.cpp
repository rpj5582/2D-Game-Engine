#include "stdafx.h"
#include "Engine.h"

#include "Window.h"

Engine::Engine(int width, int height)
{
	m_assetManager = new AssetManager();
	m_renderer = new Renderer();
	m_terrain = new Terrain(m_renderer->getVertexBufferID(), m_renderer->getIndexBufferID());
	m_camera = new Camera(width, height);

	// Call the blocks constructor to initialize all the blocks
	BlockContainer blocks;

	m_sprites = std::vector<Sprite>();
	m_sprites.push_back({ glm::vec2(0.0f, 100.0f), glm::vec2(BLOCK_SIZE), 3.14159265359f / 4.0f, m_assetManager->getTexture("dirt"), m_assetManager->getShader("defaultShader") });

	m_terrain->genStartingChunks(glm::vec2());
}

Engine::~Engine()
{
	delete m_camera;
	delete m_terrain;
	delete m_renderer;
	delete m_assetManager;
}

void Engine::update(float deltaTime)
{
	if (Input::getInstance()->isKeyHeld(GLFW_KEY_LEFT))
	{
		m_camera->translate(glm::vec2(-200.0f * deltaTime, 0.0f));
	}

	if (Input::getInstance()->isKeyHeld(GLFW_KEY_RIGHT))
	{
		m_camera->translate(glm::vec2(200.0f * deltaTime, 0.0f));
	}

	if (Input::getInstance()->isKeyHeld(GLFW_KEY_UP))
	{
		m_camera->translate(glm::vec2(0.0f, 200.0f * deltaTime));
	}

	if (Input::getInstance()->isKeyHeld(GLFW_KEY_DOWN))
	{
		m_camera->translate(glm::vec2(0.0f, -200.0f * deltaTime));
	}

	glClear(GL_COLOR_BUFFER_BIT);

	m_renderer->render(*m_camera, *m_terrain);

	if(m_sprites.size() > 0)
		m_renderer->render(*m_camera, &m_sprites[0], m_sprites.size());
}

void Engine::onWindowResize(int width, int height)
{
	m_camera->resize(width, height);
}
