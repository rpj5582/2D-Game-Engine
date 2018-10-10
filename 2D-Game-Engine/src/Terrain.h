#pragma once

#include "Blocks.h"
#include "Camera.h"
#include "SimplexNoise/SimplexNoise.h"

#include <unordered_set>
#include <future>
#include <mutex>

#define map(input, inputMin, inputMax, outputMin, outputMax) outputMin + ((outputMax - outputMin) / (inputMax - inputMin)) * (input - inputMin)

#define SMOOTHNESS 400.0f // A smoothness value used for smoothing out noise (higher is smoother)

#define CHUNK_SIZE 64 // The width (and height) of a chunk in blocks

#define HEIGHT_FLUX 768 // The height fluctuation of the surface in pixels
#define SURFACE_OCTAVES 4 // The number of octaves used in the noise function for calculating terrain height (how many noise samples are blended together)

#define STONE_OCTAVES 8 // The number of octaves used in the noise function for generating stone
#define STONE_FLUX 64 // The fluctuation in stone noise values from -STONE_FLUX to STONE_FLUX
#define STONE_WEIGHT 0 // The number the noise result must be greater than or equal to for stone to be generated

#define CHUNK_CONTAINER_DISTANCE 2 // How many chunk containers in one direction excluding the center - must be an even number

// The number of chunk containers (number of renderable chunks)
#define CHUNK_CONTAINER_SIZE (CHUNK_CONTAINER_DISTANCE + 1) * (CHUNK_CONTAINER_DISTANCE + 1)

#define CAMERA_VIEW_BUFFER_GEN 4 // Number of chunks to add to the camera's chunk when checking for chunk generation
#define CAMERA_VIEW_BUFFER_UNLOAD 8 // Number of chunks to add to the camera's edge when checking for chunks to unload
#define CAMERA_VIEW_BUFFER_CONTAINER_REASSIGN 128 // Number of pixels to add to the camera's edge when checking for chunk container reassignment

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


struct Block
{
	Sprite sprite;
	BlockType blockType;
};

enum ChunkType
{
	CHUNK_AIR,
	CHUNK_SURFACE,
	CHUNK_UNDERGROUND
};

struct Chunk
{
	Chunk(glm::vec2 chunkPosition) : chunkPosition(chunkPosition) {}

	Block blocks[CHUNK_SIZE * CHUNK_SIZE];
	unsigned int blockIndexMap[CHUNK_SIZE * CHUNK_SIZE];
	unsigned int blockCount[BLOCK_COUNT];

	std::mutex mutex;
	std::condition_variable cv;

	const glm::vec2 chunkPosition;
	ChunkType chunkType;
	int containerIndex;
	bool hasWormHead;
	bool hasGenerated;
	bool hasFullyLoaded;
};

struct ChunkContainer
{
	Chunk* chunk;
	unsigned int vao;
	unsigned int worldPositionsVBO;
	unsigned int uvOffsetIndicesVBO;
};

struct CaveWorm
{
	glm::vec3 headNoisePosition;
	glm::vec2 headChunkPosition;
	size_t length;
};

class Terrain
{
public:
	Terrain(const Camera& camera, unsigned int vertexBufferID, unsigned int indexBufferID);
	~Terrain();

	void update();

	std::vector<ChunkContainer*> getCollidingChunks(const Sprite& sprite);
	const ChunkContainer* getChunkContainers() const;

private:
	Chunk* createChunk(glm::vec2 chunkPosition);

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
	std::vector<Chunk*> genTrees(Chunk* baseChunk);
	void genTreeBranches(Chunk* chunk, int blockX, int treeHeight, float heightRatio, int direction, std::unordered_set<const Block*>& treeBlocks);

	void sortBlockIndexMap(const Block blocks[CHUNK_SIZE * CHUNK_SIZE], unsigned int blockIndexMap[CHUNK_SIZE * CHUNK_SIZE]);
	void updateDrawingBuffers(const size_t containerIndex);
	void clearWorldPositions(const ChunkContainer& chunkContainer);
	void clearUVOffsetIndices(const ChunkContainer& chunkContainer);

	void checkThreadsFinished();
	void checkShiftChunkContainers();
	void checkGenChunks();
	void checkUnloadChunks();

	void setBlock(Block& block, BlockType type, glm::vec2 position, unsigned int uvOffsetIndex);
	
	int calculateSurfaceHeight(float chunkWorldPositionX, size_t blockX);

	glm::vec2 worldToChunkCoords(glm::vec2 worldPosition) const;
	glm::vec2 chunkToWorldCoords(glm::vec2 chunkPosition) const;
	glm::vec2 snapToBlockGrid(glm::vec2 worldPosition) const;

	const Camera& m_camera;

	SimplexNoise* m_terrainNoise;
	SimplexNoise* m_treeNoise;

	std::unordered_map<glm::vec2, Chunk*> m_chunks;
	ChunkContainer m_chunkContainers[CHUNK_CONTAINER_SIZE];

	glm::vec2 m_chunkContainerOriginWorldPos;

	std::vector<Chunk*> m_queuedChunksToGen;
	std::mutex m_genQueueMutex;
	std::vector<std::future<Chunk*>> m_genChunkThreads;
	std::vector<std::future<std::vector<Chunk*>>> m_postGenThreads;
};
