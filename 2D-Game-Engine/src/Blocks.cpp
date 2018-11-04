#include "stdafx.h"
#include "Blocks.h"

std::vector<const Renderable*> BlockContainer::m_blockRenderData;

BlockContainer::BlockContainer()
{
	AssetManager* assetManager = AssetManager::getInstance();

	unsigned int terrainShaderID = assetManager->loadShader("terrainShader", "shaders/terrainVertexShader.glsl", "shaders/fragmentShader.glsl");
	unsigned int blockSpritesheet = assetManager->loadTexture("blockSpritesheet", "textures/block_spritesheet2.png");
	
	RendererSystem* renderSystem = RendererSystem::getInstance();

	// Air
	renderSystem->addComponent(0, Texture(), glm::vec2(), 0, 0, nullptr);

	// Dirt
	glm::vec2 uvOffsetsDirt[MAX_ANIMATION_LENGTH] = { glm::vec2(1, 1) };
	renderSystem->addComponent(0, Texture(glm::vec2(256), blockSpritesheet), glm::vec2(16), terrainShaderID, 0, uvOffsetsDirt);

	// Grass
	glm::vec2 uvOffsetsGrass[MAX_ANIMATION_LENGTH] = { glm::vec2(0, 0), glm::vec2(0, 1), glm::vec2(0, 2), glm::vec2(1, 2), glm::vec2(2, 2), glm::vec2(2, 1), glm::vec2(2, 0), glm::vec2(1, 0) };
	renderSystem->addComponent(0, Texture(glm::vec2(256), blockSpritesheet), glm::vec2(16), terrainShaderID, 0, uvOffsetsGrass);
	
	// Stone
	glm::vec2 uvOffsetsStone[MAX_ANIMATION_LENGTH] = { glm::vec2(4, 1) };
	renderSystem->addComponent(0, Texture(glm::vec2(256), blockSpritesheet), glm::vec2(16), terrainShaderID, 0, uvOffsetsStone);

	// Wood
	glm::vec2 uvOffsetsWood[MAX_ANIMATION_LENGTH] = { glm::vec2(6, 1) };
	renderSystem->addComponent(0, Texture(glm::vec2(256), blockSpritesheet), glm::vec2(16), terrainShaderID, 0, uvOffsetsWood);

	// Branch
	glm::vec2 uvOffsetsBranch[MAX_ANIMATION_LENGTH] = { glm::vec2(6, 1) };
	renderSystem->addComponent(0, Texture(glm::vec2(256), blockSpritesheet), glm::vec2(16), terrainShaderID, 0, uvOffsetsBranch);

	// Leaf
	glm::vec2 uvOffsetsLeaf[MAX_ANIMATION_LENGTH] = { glm::vec2(6, 2) };
	renderSystem->addComponent(0, Texture(glm::vec2(256), blockSpritesheet), glm::vec2(16), terrainShaderID, 0, uvOffsetsLeaf);

	m_blockRenderData = renderSystem->getComponents(0);
}

BlockContainer::~BlockContainer()
{
}

const Renderable& BlockContainer::getBlockRenderData(BlockType name)
{
	assert((int)name >= 0 && (int)name < m_blockRenderData.size());

	return *m_blockRenderData[name];
}
