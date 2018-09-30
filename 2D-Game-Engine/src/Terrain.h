#pragma once

#include "Blocks.h"
#include "Camera.h"
#include "SimplexNoise/SimplexNoise.h"

#include <future>
#include <mutex>

#define map(input, inputMin, inputMax, outputMin, outputMax) outputMin + ((outputMax - outputMin) / (inputMax - inputMin)) * (input - inputMin)

#define CHUNK_SIZE 64 // The width (and height) of a chunk in blocks

#define HEIGHT_FLUX 768 // The height fluctuation of the surface in pixels
#define SMOOTHNESS 400.0f // The smoothness value of the surface (higher is smoother)
#define SURFACE_OCTAVES 4 // The number of octaves used in the noise function for calculating terrain height (how many noise samples are blended together)

#define STONE_OCTAVES 8 // The number of octaves used in the noise function for generating stone
#define STONE_FLUX 64 // The fluctuation in stone noise values from -STONE_FLUX to STONE_FLUX
#define STONE_WEIGHT 0 // The number the noise result must be greater than or equal to for stone to be generated

#define CHUNK_VIEW_DISTANCE 2 // How many chunks in one direction excluding the center chunk - must be an even number

// The number of visible chunks
#define CHUNK_VIEW_SIZE (CHUNK_VIEW_DISTANCE + 1) * (CHUNK_VIEW_DISTANCE + 1)

#define CAMERA_VIEW_BUFFER 32 // Number of pixels to add to the camera's edge when checking for chunk generation

#define TERRAIN_CHUNK_HEIGHT 8 // The number of vertical chunks in the terrain

#define CAVE_WORM_LENGTH_MIN 32 // The minimum number of worm segments used for cave generation
#define CAVE_WORM_LENGTH_MAX 512 // The maximum number of worm segments used for cave generation

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
	Block blocks[CHUNK_SIZE * CHUNK_SIZE];
	unsigned int vao;
	unsigned int worldPositionsVBO;
	unsigned int uvOffsetIndicesVBO;
	unsigned int blockCount[BLOCK_COUNT];
	ChunkType chunkType;
	bool hasLoaded;
};

class Terrain
{
public:
	Terrain(int seed, const Camera& camera, unsigned int vertexBufferID, unsigned int indexBufferID);
	~Terrain();

	void update();

	std::vector<Chunk*> getCollidingChunks(const Sprite& sprite);
	Chunk* getVisibleChunks() const;

private:
	void genStartingChunks(glm::vec2 startingPosition);
	void genChunks();
	void queueGenChunks(const std::vector<std::pair<glm::vec2, Chunk*>>& chunkInfo);

	Chunk* genChunkThreaded(glm::vec2 chunkWorldPosition, Chunk* chunk);

	void updateGrassBlocks(glm::vec2 chunkWorldPosition, Block* blocks);
	void genCave(ChunkType chunkType, Block* blocks, unsigned int blockCount[BLOCK_COUNT], glm::vec2 chunkWorldPosition);
	glm::vec2* genCaveWorm(glm::vec2 chunkWorldPosition);

	void updateWorldPositions(const Chunk* chunk);
	void updateUVOffsetIndices(const Chunk* chunk);

	void checkGenChunks();

	void setBlock(Block& block, BlockType type, glm::vec2 position, unsigned int uvOffsetIndex);

	glm::vec2 worldToChunkCoords(glm::vec2 worldPosition) const;
	glm::vec2 chunkToWorldCoords(glm::vec2 chunkPosition) const;
	glm::vec2 snapToBlockGrid(glm::vec2 worldPosition) const;

	int m_seed;

	const Camera& m_camera;

	SimplexNoise* m_noise;

	Chunk* m_visibleChunks;
	std::unordered_map<unsigned int, Chunk*> m_visibleChunkMap;

	glm::vec2 m_chunkOriginWorldPos;

	std::vector<std::pair<glm::vec2, Chunk*>> m_chunkInfo;
	std::vector<std::future<Chunk*>> m_genChunkThreads;
	std::mutex m_mutex;

	glm::vec2* m_caveWorm;
	size_t m_caveWormLength;
};
