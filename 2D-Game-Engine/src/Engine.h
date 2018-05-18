#pragma once

#include "AssetManager.h"
#include "Renderer.h"

class Engine
{
public:
	Engine(int width, int height);
	~Engine();

	void update(float deltaTime);

	void onWindowResize(int width, int height);

private:
	AssetManager* m_assetManager;
	Renderer* m_renderer;
	Terrain* m_terrain;

	Camera* m_camera;
	std::vector<Sprite> m_sprites;
};