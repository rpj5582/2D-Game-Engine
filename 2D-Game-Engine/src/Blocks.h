#pragma once

#include "AssetManager.h"
#include "Sprite.h"

#define BLOCK_SIZE 16 // The size of a block in pixels
#define BLOCK_COUNT 7 // The number of different blocks

enum BlockType
{
	AIR,
	DIRT,
	GRASS,
	STONE,
	WOOD,
	BRANCH,
	LEAF
};

class BlockContainer
{
public:
	BlockContainer();
	~BlockContainer();

	static const RenderData& getBlockRenderData(BlockType name);

private:
	static std::unordered_map<BlockType, RenderData> blockRenderData;
};