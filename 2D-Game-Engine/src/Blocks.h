#pragma once

#include "AssetManager.h"
#include "Components/Transform.h"
#include "Systems/RenderSystem.h"
#include "Systems/PhysicsSystem.h"

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

struct Block
{
	Block() : transform(Transform(0)), renderable(Renderable(0)), blockType(AIR) {}

	Transform transform;
	Renderable renderable;
	BlockType blockType;
};

class BlockContainer
{
public:
	BlockContainer();
	~BlockContainer();

	static const Renderable& getBlockRenderData(BlockType name);

private:
	static std::vector<const Renderable*> m_blockRenderData;
};