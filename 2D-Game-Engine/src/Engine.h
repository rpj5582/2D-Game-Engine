#pragma once

#include "AssetManager.h"
#include "Systems/Systems.h"
#include "Terrain.h"
#include "PlayerController.h"

class Engine
{
public:
	Engine(int width, int height);
	~Engine();

	void update(float deltaTime);

	void onWindowResize(int width, int height);

private:
	AssetManager* m_assetManager;
	TransformSystem* m_transformSystem;
	RendererSystem* m_renderSystem;
	PhysicsSystem* m_physicsSystem;
	Terrain* m_terrain;
	Camera* m_camera;
	PlayerController* m_playerController;

#ifdef _DEBUG
	DebugDrawPhysics* m_debugDraw;
	bool m_shouldDrawDebugPhysics;
#endif 
};