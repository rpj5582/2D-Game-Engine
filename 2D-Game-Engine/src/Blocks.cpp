#include "stdafx.h"
#include "Blocks.h"

std::unordered_map<BlockType, RenderData> BlockContainer::blockRenderData = std::unordered_map<BlockType, RenderData>();

BlockContainer::BlockContainer()
{
	AssetManager* assetManager = AssetManager::getInstance();

	unsigned int terrainShaderID = assetManager->loadShader("terrainShader", "shaders/terrainVertexShader.glsl", "shaders/fragmentShader.glsl");
	unsigned int blockSpritesheet = assetManager->loadTexture("blockSpritesheet", "textures/block_spritesheet2.png");

	blockRenderData[AIR] = {};

	glm::vec2 uvOffsetsDirt[MAX_ANIMATION_LENGTH] = { glm::vec2(1, 2) };
	blockRenderData[DIRT] = RenderData(Texture(glm::vec2(256), blockSpritesheet), terrainShaderID, glm::vec2(16), 0, uvOffsetsDirt);

	glm::vec2 uvOffsetsGrass[MAX_ANIMATION_LENGTH] = { glm::vec2(0, 0), glm::vec2(0, 1), glm::vec2(0, 3), glm::vec2(1, 3), glm::vec2(3, 3), glm::vec2(3, 2), glm::vec2(3, 0), glm::vec2(1, 0) };
	blockRenderData[GRASS] = RenderData(Texture(glm::vec2(256), blockSpritesheet), terrainShaderID, glm::vec2(16), 2, uvOffsetsGrass);
	
	glm::vec2 uvOffsetsStone[MAX_ANIMATION_LENGTH] = { glm::vec2(5, 2) };
	blockRenderData[STONE] = RenderData(Texture(glm::vec2(256), blockSpritesheet), terrainShaderID, glm::vec2(16), 0, uvOffsetsStone);

	glm::vec2 uvOffsetsWood[MAX_ANIMATION_LENGTH] = { glm::vec2(8, 1) };
	blockRenderData[WOOD] = RenderData(Texture(glm::vec2(256), blockSpritesheet), terrainShaderID, glm::vec2(16), 0, uvOffsetsWood);

	glm::vec2 uvOffsetsBranch[MAX_ANIMATION_LENGTH] = { glm::vec2(8, 1) };
	blockRenderData[BRANCH] = RenderData(Texture(glm::vec2(256), blockSpritesheet), terrainShaderID, glm::vec2(16), 0, uvOffsetsBranch);

	glm::vec2 uvOffsetsLeaf[MAX_ANIMATION_LENGTH] = { glm::vec2(8, 2) };
	blockRenderData[LEAF] = RenderData(Texture(glm::vec2(256), blockSpritesheet), terrainShaderID, glm::vec2(16), 0, uvOffsetsLeaf);
}

BlockContainer::~BlockContainer()
{
}

const RenderData& BlockContainer::getBlockRenderData(BlockType name)
{
	assert(blockRenderData.find(name) != blockRenderData.end());
	return blockRenderData[name];
}
