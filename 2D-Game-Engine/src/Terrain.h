#pragma once

#include "Blocks.h"
#include "TerrainRenderer.h"

#include "SimplexNoise/SimplexNoise.h"

#include <future>
#include <mutex>

#define map(input, inputMin, inputMax, outputMin, outputMax) outputMin + ((outputMax - outputMin) / (inputMax - inputMin)) * (input - inputMin)

#define SMOOTHNESS 400.0f // A smoothness value used for smoothing out noise (higher is smoother)
#define TERRAIN_SMOOTHESS 800.0f  // A smoothness value used for smoothing out terrain noise (higher is smoother)

#define CHUNK_SIZE 64 // The width (and height) of a chunk in blocks

#define HEIGHT_FLUX 1536 // The height fluctuation of the surface in pixels
#define SURFACE_OCTAVES 4 // The number of octaves used in the noise function for calculating terrain height (how many noise samples are blended together)

#define STONE_OCTAVES 8 // The number of octaves used in the noise function for generating stone
#define STONE_FLUX 64 // The fluctuation in stone noise values from -STONE_FLUX to STONE_FLUX
#define STONE_WEIGHT 0 // The number the noise result must be greater than or equal to for stone to be generated

#define CAMERA_VIEW_BUFFER_GEN 4 // Number of chunks to add to the camera's chunk when checking for chunk generation
#define CAMERA_VIEW_BUFFER_UNLOAD 8 // Number of chunks to add to the camera's edge when checking for chunks to unload

#define TERRAIN_CHUNK_HEIGHT 16 // The number of vertical chunks in the terrain

#define CAVE_WORM_LENGTH_MIN 512 // The minimum number of worm segments used for cave generation
#define CAVE_WORM_LENGTH_MAX 1536 // The maximum number of worm segments used for cave generation
#define CAVE_WORM_RADIUS_MIN 0 // The minimum number of blocks to carve out from the center point
#define CAVE_WORM_RADIUS_MAX 6 // The maximum number of blocks to carve out from the center point

#define TREE_OCTAVES 4  // The number of octaves used in the noise function for calculating tree frequency
#define TREE_SMOOTHNESS 64.0f // The smoothness value used in scaling the tree noise (higher is smoother)
#define TREE_BASE_HEIGHT_MIN 8 // The minimum height of the tree in blocks
#define TREE_BASE_HEIGHT_MAX 24 // The maximum height of the tree in blocks
#define TREE_TOP_LEAF_RADIUS_MIN 3 // The minimum radius of leaf blocks extending from the top of the tree
#define TREE_TOP_LEAF_RADIUS_MAX 5 // The maximum radius of leaf blocks extending from the top of the tree
#define TREE_BRANCH_LENGTH_MIN 0 // The minimum length of a branch on a tree
#define TREE_BRANCH_LENGTH_MAX 8 // The maximum length of a branch on a tree
#define TREE_BRANCH_HEIGHT_MIN 4 // The minimum number of blocks above the base of the tree that branches will start to generate
#define TREE_BRANCH_HEIGHT_MAX 10 // The maximum number of blocks above the base of the tree that branches will start to generate

enum ChunkType
{
	CHUNK_AIR,
	CHUNK_SURFACE,
	CHUNK_UNDERGROUND
};

struct Chunk
{
	Chunk(glm::vec2 chunkPosition) : chunkPosition(chunkPosition), physicsObject(PhysicsObject(0)) {}

	Block blocks[CHUNK_SIZE * CHUNK_SIZE];
	unsigned int blockIndexMap[CHUNK_SIZE * CHUNK_SIZE];
	unsigned int blockCount[BLOCK_COUNT];

	std::mutex mutex;
	std::condition_variable cv;
	
	PhysicsObject physicsObject;

	const glm::vec2 chunkPosition;
	ChunkType chunkType;
	int containerIndex;
	bool hasWormHead;
	bool hasGenerated;
	bool hasFullyLoaded;
};

class Terrain
{
public:
	Terrain(b2World& physicsWorld, glm::vec2 startingPosition, unsigned int vertexBufferID, unsigned int indexBufferID);
	~Terrain();

	Chunk* createChunk(glm::vec2 chunkPosition);
	Chunk* getChunk(glm::vec2 chunkPosition) const;

	void cameraUpdate(const Camera& camera);
	void update();

	void render(const Camera& camera) const;

	static glm::vec2 worldToChunkCoords(glm::vec2 worldPosition);
	static glm::vec2 chunkToWorldCoords(glm::vec2 chunkPosition);
	static glm::vec2 snapToBlockGrid(glm::vec2 worldPosition);

private:
	void genStartingChunks(glm::vec2 startingPosition);
	void genChunks();
	void queueGenChunk(Chunk* chunk);
	void queueGenChunks(const std::vector<Chunk*>& chunks);

	Chunk* genChunkThreaded(Chunk* chunk);
	std::vector<Chunk*> postGenChunkThreaded(Chunk* chunk);

	void unloadChunks();

	void updateGrassBlocks(glm::vec2 chunkWorldPosition, Block* blocks);

	void genCave(Block* blocks, unsigned int blockCount[BLOCK_COUNT], glm::vec2 chunkWorldPosition);
	std::vector<Chunk*> genCaveWorm(glm::vec2 chunkPosition);
	void genTrees(Chunk* baseChunk);

	void sortBlockIndexMap(const Block blocks[CHUNK_SIZE * CHUNK_SIZE], unsigned int blockIndexMap[CHUNK_SIZE * CHUNK_SIZE]);

	void checkThreadsFinished();
	void checkGenChunks(const Camera& camera);
	void checkUnloadChunks(const Camera& camera);

	void setBlock(Block& block, BlockType type, glm::vec2 position, unsigned int uvOffsetIndex);
	
	int calculateSurfaceHeight(float chunkWorldPositionX, size_t blockX);

	TerrainRenderer* m_terrainRenderer;

	b2World& m_physicsWorld;

	SimplexNoise* m_terrainNoise;
	SimplexNoise* m_treeNoise;

	std::unordered_map<glm::vec2, Chunk*> m_chunks;

	std::vector<Chunk*> m_queuedChunksToGen;
	std::mutex m_genQueueMutex;

	std::vector<std::future<Chunk*>> m_genChunkThreads;
	std::vector<std::future<std::vector<Chunk*>>> m_postGenThreads;

	static std::vector<std::vector<std::vector<BlockType>>> s_treePatterns;
};
