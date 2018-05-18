#pragma once

#include "Blocks.h"
#include "SimplexNoise/SimplexNoise.h"

#define CHUNK_SIZE 64 // The size of a chunk in blocks
#define HEIGHT_FLUX 256 // The height fluctuation of the surface in pixels
#define SMOOTHNESS 400.0f // The smoothness value of the surface (higher is smoother)
#define OCTAVES 4 // The number of octaves used in the noise function (how many noise samples are blended together)
#define CHUNK_VIEW_DISTANCE 2 // How many chunks from the center chunk to the edge of the screen - must be an even number

// The number of visible chunks
#define CHUNK_VIEW_SIZE (CHUNK_VIEW_DISTANCE + 1) * (CHUNK_VIEW_DISTANCE + 1)

struct Chunk
{
	unsigned int vbo;
	Sprite sprites[CHUNK_SIZE * CHUNK_SIZE];
	unsigned int blockCount[BLOCK_COUNT];
};

class Terrain
{
public:
	Terrain(unsigned int vertexBufferID, unsigned int indexBufferID);
	~Terrain();

	unsigned int getVAO() const;

	const Chunk* getVisibleChunks() const;

	void genStartingChunks(glm::vec2 startingPosition);

private:
	void genChunk(ptrdiff_t x, ptrdiff_t y, size_t index);
	void setBlock(Sprite& sprite, Blocks type, glm::vec2 position);

	void worldToChunkCoords(glm::vec2 worldPosition, ptrdiff_t* x, ptrdiff_t* y) const;
	glm::vec2 chunkToWorldCoords(ptrdiff_t x, ptrdiff_t y) const;

	SimplexNoise* m_noise;

	Chunk* m_visibleChunks;
	unsigned int m_vao;
};