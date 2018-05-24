#include "stdafx.h"
#include "Blocks.h"

std::unordered_map<Blocks, RenderData> BlockContainer::blocks = std::unordered_map<Blocks, RenderData>();

BlockContainer::BlockContainer()
{
	AssetManager* assetManager = AssetManager::getInstance();

	unsigned int terrainShaderID = assetManager->loadShader("terrainShader", "shaders/terrainVertexShader.glsl", "shaders/fragmentShader.glsl");
	unsigned int dirtTextureID = assetManager->loadTexture("dirt", "textures/test.png");
	unsigned int grassTextureID = assetManager->loadTexture("grass", "textures/100_final_grass_texture.png");
	unsigned int stoneTextureID = assetManager->loadTexture("stone", "textures/rock.png");

	blocks[AIR] = { 0, 0 };
	blocks[DIRT] = { dirtTextureID, terrainShaderID };
	blocks[GRASS] = { grassTextureID, terrainShaderID };
	blocks[STONE] = { stoneTextureID, terrainShaderID };
}

BlockContainer::~BlockContainer()
{
}

const RenderData& BlockContainer::getBlock(Blocks name)
{
	assert(blocks.find(name) != blocks.end());
	return blocks[name];
}
