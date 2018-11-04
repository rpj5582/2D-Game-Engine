#include "stdafx.h"
#include "Engine.h"

#include "Window.h"

Engine::Engine(int width, int height)
{
	// Seed the simplex noise generator
	SimplexNoise::init(0);

	m_assetManager = new AssetManager();

	// Construct the systems
	m_transformSystem = new TransformSystem();
	m_renderSystem = new RendererSystem();
	m_physicsSystem = new PhysicsSystem();

	// Call the blocks constructor to initialize all the blocks
	BlockContainer blocks;

	m_camera = new Camera(width, height);
	m_terrain = new Terrain(m_physicsSystem->getWorld(), m_camera->getPosition(), m_renderSystem->getVertexBufferID(), m_renderSystem->getIndexBufferID());

	// Create player
	AssetManager* assetManager = AssetManager::getInstance();
	unsigned int playerTextureID = assetManager->loadTexture("player", "textures/player.png");
	unsigned int defaultShaderID = assetManager->getShader("defaultShader");

	size_t playerID = 1;
	m_playerController = new PlayerController(playerID);

	TransformSystem::getInstance()->addComponent(playerID, glm::vec2(), glm::vec2(32, 64));
	RendererSystem::getInstance()->addComponent(playerID, Texture(glm::vec2(32, 64), playerTextureID), glm::vec2(32, 64), defaultShaderID, 0, nullptr);
	
	size_t playerPhysicsObjectIndex = PhysicsSystem::getInstance()->addComponent(playerID, glm::vec2(), glm::vec2(32, 64), b2_dynamicBody);
	PhysicsSystem::getInstance()->addFixture(playerID, playerPhysicsObjectIndex, glm::vec2(0, -32), glm::vec2(32, 4), true,
		std::bind(&PlayerController::beginContact, m_playerController, std::placeholders::_1, std::placeholders::_2),
		std::bind(&PlayerController::endContact, m_playerController, std::placeholders::_1, std::placeholders::_2));
	PhysicsSystem::getInstance()->addFixture(playerID, playerPhysicsObjectIndex, glm::vec2(), glm::vec2(32, 64));


	// TEMP
	size_t tempID = 2;
	TransformSystem::getInstance()->addComponent(tempID, glm::vec2(0, -100), glm::vec2(2048, 64));
	RendererSystem::getInstance()->addComponent(tempID, Texture(glm::vec2(32, 64), playerTextureID), glm::vec2(32, 64), defaultShaderID, 0, nullptr);
	
	size_t tempPhysicsObjectIndex = PhysicsSystem::getInstance()->addComponent(tempID, glm::vec2(0, -100), glm::vec2(2048, 64), b2_kinematicBody);
	PhysicsSystem::getInstance()->addFixture(tempID, tempPhysicsObjectIndex, glm::vec2(), glm::vec2(2048, 64));

#ifdef _DEBUG
	m_debugDraw = new DebugDrawPhysics(*m_camera);
	m_debugDraw->SetFlags(b2Draw::e_shapeBit);
	m_physicsSystem->setDebugDraw(m_debugDraw);
	m_shouldDrawDebugPhysics = false;
#endif 
}

Engine::~Engine()
{
#ifdef _DEBUG
	delete m_debugDraw;
#endif 

	delete m_playerController;
	delete m_terrain;
	delete m_camera;
	delete m_physicsSystem;
	delete m_renderSystem;
	delete m_transformSystem;
	delete m_assetManager;
}

void Engine::update(float deltaTime)
{
	m_playerController->update(deltaTime);

	m_terrain->cameraUpdate(*m_camera);
	m_terrain->update();

	m_physicsSystem->update();

#ifdef _DEBUG
	if (Input::getInstance()->isKeyPressed(GLFW_KEY_Q))
	{
		m_shouldDrawDebugPhysics = !m_shouldDrawDebugPhysics;
	}
#endif

	glClear(GL_COLOR_BUFFER_BIT);

	const Transform* playerTransform = TransformSystem::getInstance()->getComponent(m_playerController->getPlayerID());
	if (playerTransform)
		m_camera->translate(playerTransform->position - m_camera->getPosition());
	
	m_terrain->render(*m_camera);
	m_renderSystem->render(*m_camera);

#ifdef _DEBUG
	if (m_shouldDrawDebugPhysics)
	{
		m_debugDraw->clearInstanceData();
		m_physicsSystem->drawDebugData();
		m_debugDraw->draw();
	}
#endif
}

void Engine::onWindowResize(int width, int height)
{
	m_camera->resize(width, height);
}
