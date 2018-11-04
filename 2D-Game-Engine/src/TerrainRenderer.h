#pragma once

#include "Camera.h"

#define CHUNK_CONTAINER_DISTANCE 2 // How many chunk containers in one direction excluding the center - must be an even number

// The number of chunk containers (number of renderable chunks)
#define CHUNK_CONTAINER_SIZE (CHUNK_CONTAINER_DISTANCE + 1) * (CHUNK_CONTAINER_DISTANCE + 1)

#define CAMERA_VIEW_BUFFER_CONTAINER_REASSIGN 128 // Number of pixels to add to the camera's edge when checking for chunk container reassignment

class Terrain;
struct Chunk;

struct ChunkContainer
{
	Chunk* chunk;
	unsigned int vao;
	unsigned int worldPositionsVBO;
	unsigned int uvOffsetIndicesVBO;
};

class TerrainRenderer
{
public:
	TerrainRenderer(Terrain* terrain, unsigned int vertexBufferID, unsigned int indexBufferID);
	~TerrainRenderer();

	void initChunkContainers(std::vector<Chunk*> chunks, glm::vec2 startingChunkPosition);
	std::vector<Chunk*> checkShiftChunkContainers(const Camera& camera);

	void update(const Camera& camera);

	void updateDrawingBuffers(const size_t containerIndex);

	void render(const Camera& camera) const;

private:
	void clearWorldPositions(const ChunkContainer& chunkContainer);
	void clearUVOffsetIndices(const ChunkContainer& chunkContainer);

	Terrain* m_terrain;
	unsigned int m_vertexBufferID;
	unsigned int m_indexBufferID;

	ChunkContainer m_chunkContainers[CHUNK_CONTAINER_SIZE];
	glm::vec2 m_chunkContainerOriginWorldPos;
};
