#include "stdafx.h"
#include "Engine.h"

#include "Window.h"

Engine::Engine(int width, int height)
{
	m_assetManager = new AssetManager();
	m_renderer = new Renderer();

	// Call the blocks constructor to initialize all the blocks
	BlockContainer blocks;

	m_camera = new Camera(width, height);
	m_terrain = new Terrain(0, *m_camera, m_renderer->getVertexBufferID(), m_renderer->getIndexBufferID());

	m_playerController = new PlayerController(Transform(glm::vec2(), glm::vec2(32, 64)));

	m_collisionHandler = new CollisionHandler();
}

Engine::~Engine()
{
	delete m_collisionHandler;
	delete m_playerController;
	delete m_terrain;
	delete m_camera;
	delete m_renderer;
	delete m_assetManager;
}

void Engine::update(float deltaTime)
{
	Sprite temp = Sprite(
		Transform(glm::vec2(0, 100), glm::vec2(32, 64)),
		RenderData(Texture(glm::vec2(32, 64), AssetManager::getInstance()->getTexture("player")), AssetManager::getInstance()->getShader("defaultShader"), glm::vec2(32, 64), 0, nullptr)
	);

	m_playerController->update(deltaTime);
	m_terrain->update();

	//m_collisionHandler->clearGrid();

	//// Entity-entity collision
	//std::vector<Sprite*> sprites = { m_playerController->getSprite(), &temp };

	//m_collisionHandler->insertIntoGrid(&sprites[0], sprites.size());

	//std::vector<CollisionPair>& collisionPairs = m_collisionHandler->getCollisionPairs();
	//std::unordered_map<CollisionPair, bool> pairCheckedMap;
	//for (size_t i = 0; i < collisionPairs.size(); i++)
	//{
	//	auto it = pairCheckedMap.find(collisionPairs[i]);
	//	if (it != pairCheckedMap.end())
	//	{
	//		if (it->second) continue;
	//	}

	//	glm::vec2 mtv;
	//	if (m_collisionHandler->calculateResolution(collisionPairs[i], &mtv))
	//	{
	//		collisionPairs[i].sprite1.transform.translate(mtv);
	//		pairCheckedMap[collisionPairs[i]] = true;
	//	}
	//}

	//// Player-terrain collision
	//Sprite* playerSprite = m_playerController->getSprite();
	//std::vector<Chunk*> collidingChunks = m_terrain->getCollidingChunks(*playerSprite);
	//for (size_t i = 0; i < collidingChunks.size(); i++)
	//{
	//	Chunk* chunk = collidingChunks[i];
	//	for (size_t j = 0; j < CHUNK_SIZE * CHUNK_SIZE; j++)
	//	{
	//		Block& block = chunk->blocks[j];
	//		if (block.blockType == AIR)
	//			continue;

	//		glm::vec2 mtv;
	//		CollisionPair collisionPair = { *playerSprite, chunk->blocks[j].sprite };
	//		if (m_collisionHandler->calculateResolution(collisionPair, 1, &mtv))
	//		{
	//			m_playerController->translate(mtv);
	//		}
	//	}
	//}

	m_camera->translate(m_playerController->getPosition() - m_camera->getPosition());

	glClear(GL_COLOR_BUFFER_BIT);

	m_renderer->render(*m_camera, *m_terrain);
	m_renderer->render(*m_camera, m_playerController->getSprite(), 1);
	m_renderer->render(*m_camera, &temp, 1);
}

void Engine::onWindowResize(int width, int height)
{
	m_camera->resize(width, height);
}
