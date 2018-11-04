#include "stdafx.h"
#include "TerrainRenderer.h"

#include "Terrain.h"

#include <GL/glew.h>

#ifdef _DEBUG
#include "Input.h"
#endif

TerrainRenderer::TerrainRenderer(Terrain* terrain, unsigned int vertexBufferID, unsigned int indexBufferID)
	: m_terrain(terrain), m_vertexBufferID(vertexBufferID), m_indexBufferID(indexBufferID)
{
}

TerrainRenderer::~TerrainRenderer()
{
	for (size_t i = 0; i < CHUNK_CONTAINER_SIZE; i++)
	{
		glDeleteBuffers(1, &m_chunkContainers[i].worldPositionsVBO);
		glDeleteBuffers(1, &m_chunkContainers[i].uvOffsetIndicesVBO);
		glDeleteVertexArrays(1, &m_chunkContainers[i].vao);
	}
}

void TerrainRenderer::initChunkContainers(std::vector<Chunk*> chunks, glm::vec2 startingChunkPosition)
{
	memset(m_chunkContainers, 0, sizeof(ChunkContainer) * CHUNK_CONTAINER_SIZE);

	glm::vec2 worldPositions[CHUNK_SIZE * CHUNK_SIZE] = {};
	unsigned int uvOffsetIndices[CHUNK_SIZE * CHUNK_SIZE] = {};

	// Initialize rendering data
	for (unsigned int i = 0; i < CHUNK_CONTAINER_SIZE; i++)
	{
		// VAO
		glGenVertexArrays(1, &m_chunkContainers[i].vao);
		glBindVertexArray(m_chunkContainers[i].vao);

		// Vertices and indices
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferID);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));

		// World positions
		glGenBuffers(1, &m_chunkContainers[i].worldPositionsVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_chunkContainers[i].worldPositionsVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * CHUNK_SIZE * CHUNK_SIZE, &worldPositions[0][0], GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
		glVertexAttribDivisor(2, 1);

		// UV offset indices
		glGenBuffers(1, &m_chunkContainers[i].uvOffsetIndicesVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_chunkContainers[i].uvOffsetIndicesVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int) * CHUNK_SIZE * CHUNK_SIZE, &uvOffsetIndices[0], GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(unsigned int), (void*)0);
		glVertexAttribDivisor(3, 1);

		m_chunkContainers[i].chunk = chunks[i];
	}

	// Calculates the bottom left origin of the chunk containers
	m_chunkContainerOriginWorldPos = startingChunkPosition - glm::vec2(CHUNK_CONTAINER_DISTANCE / 2.0f);
	m_chunkContainerOriginWorldPos = Terrain::chunkToWorldCoords(m_chunkContainerOriginWorldPos);
}

std::vector<Chunk*> TerrainRenderer::checkShiftChunkContainers(const Camera& camera)
{
	glm::vec2 cameraPosition = camera.getPosition();
	int cameraWidth = camera.getWidth();
	int cameraHeight = camera.getHeight();

	// List of chunks that need to be generated
	std::vector<Chunk*> chunks;

	// Check if we need to update the chunk containers from the right
	if (cameraPosition.x + cameraWidth * 0.5f + CAMERA_VIEW_BUFFER_CONTAINER_REASSIGN >= m_chunkContainerOriginWorldPos.x + CHUNK_SIZE * BLOCK_SIZE * (CHUNK_CONTAINER_DISTANCE + 1))
	{
		// Converts the chunk containers' origin to chunk coordinates
		glm::vec2 chunkContainerOriginPos = Terrain::worldToChunkCoords(m_chunkContainerOriginWorldPos);

		// Unassign the leftmost chunks from the chunk containers
		for (size_t i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			ChunkContainer& chunkContainer = m_chunkContainers[i * (CHUNK_CONTAINER_DISTANCE + 1)];
			chunkContainer.chunk->containerIndex = -1;
			chunkContainer.chunk = nullptr;

			clearWorldPositions(chunkContainer);
			clearUVOffsetIndices(chunkContainer);
		}

		// Reassign the existing chunk containers
		size_t chunkContainerIndex = 0;
		for (size_t j = 0; j < CHUNK_CONTAINER_DISTANCE + 1; j++)
		{
			chunkContainerIndex++;

			for (size_t i = 0; i < CHUNK_CONTAINER_DISTANCE; i++)
			{
				ChunkContainer& chunkContainer = m_chunkContainers[chunkContainerIndex];
				chunkContainer.chunk->containerIndex -= 1;
				m_chunkContainers[chunkContainerIndex - 1].chunk = chunkContainer.chunk;

				if (chunkContainer.chunk->hasFullyLoaded)
					updateDrawingBuffers(chunkContainerIndex - 1);

				chunkContainerIndex++;
			}
		}

		// Assign the rightmost containers, or generate them if necessary
		for (int i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			glm::vec2 chunkPosition = glm::vec2(chunkContainerOriginPos.x + (CHUNK_CONTAINER_DISTANCE + 1), chunkContainerOriginPos.y + i);
			Chunk* chunk = m_terrain->getChunk(chunkPosition);
			if (!chunk)
			{
				chunk = m_terrain->createChunk(chunkPosition);
				chunks.push_back(chunk);
			}

			chunk->containerIndex = CHUNK_CONTAINER_DISTANCE + (CHUNK_CONTAINER_DISTANCE + 1) * i;

			ChunkContainer& chunkContainer = m_chunkContainers[chunk->containerIndex];
			chunkContainer.chunk = chunk;

			if (chunk->hasFullyLoaded)
				updateDrawingBuffers(chunk->containerIndex);
		}

		// Shift the chunk container origin by 1 chunk
		m_chunkContainerOriginWorldPos.x += CHUNK_SIZE * BLOCK_SIZE;
	}

	// Check if we need to update the chunk containers from the left
	if (cameraPosition.x - cameraWidth * 0.5f - CAMERA_VIEW_BUFFER_CONTAINER_REASSIGN <= m_chunkContainerOriginWorldPos.x)
	{
		// Converts the chunk containers' origin to chunk coordinates
		glm::vec2 chunkContainerOriginPos = Terrain::worldToChunkCoords(m_chunkContainerOriginWorldPos);

		// Unassign the rightmost chunks from the chunk containers
		for (size_t i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			ChunkContainer& chunkContainer = m_chunkContainers[CHUNK_CONTAINER_DISTANCE + (CHUNK_CONTAINER_DISTANCE + 1) * i];
			chunkContainer.chunk->containerIndex = -1;
			chunkContainer.chunk = nullptr;

			clearWorldPositions(chunkContainer);
			clearUVOffsetIndices(chunkContainer);
		}

		// Reassign the existing chunk containers
		size_t chunkContainerIndex = CHUNK_CONTAINER_SIZE - 1;
		for (size_t j = 0; j < CHUNK_CONTAINER_DISTANCE + 1; j++)
		{
			chunkContainerIndex--;

			for (size_t i = 0; i < CHUNK_CONTAINER_DISTANCE; i++)
			{
				ChunkContainer& chunkContainer = m_chunkContainers[chunkContainerIndex];
				chunkContainer.chunk->containerIndex += 1;
				m_chunkContainers[chunkContainerIndex + 1].chunk = chunkContainer.chunk;

				if (chunkContainer.chunk->hasFullyLoaded)
					updateDrawingBuffers(chunkContainerIndex + 1);

				chunkContainerIndex--;
			}
		}

		// Assign the leftmost containers, or generate them if necessary
		for (int i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			glm::vec2 chunkPosition = glm::vec2(chunkContainerOriginPos.x - 1, chunkContainerOriginPos.y + i);
			Chunk* chunk = m_terrain->getChunk(chunkPosition);
			if (!chunk)
			{
				chunk = m_terrain->createChunk(chunkPosition);
				chunks.push_back(chunk);
			}

			chunk->containerIndex = i * (CHUNK_CONTAINER_DISTANCE + 1);

			ChunkContainer& chunkContainer = m_chunkContainers[chunk->containerIndex];
			chunkContainer.chunk = chunk;

			if (chunk->hasFullyLoaded)
				updateDrawingBuffers(chunk->containerIndex);
		}

		// Shift the chunk container origin by 1 chunk
		m_chunkContainerOriginWorldPos.x -= CHUNK_SIZE * BLOCK_SIZE;
	}

	// Check if we need to update the chunk containers from the top
	if (cameraPosition.y + cameraHeight * 0.5f + CAMERA_VIEW_BUFFER_CONTAINER_REASSIGN >= m_chunkContainerOriginWorldPos.y + CHUNK_SIZE * BLOCK_SIZE * (CHUNK_CONTAINER_DISTANCE + 1) && cameraPosition.y + cameraHeight * 0.5f <= TERRAIN_CHUNK_HEIGHT * CHUNK_SIZE * BLOCK_SIZE)
	{
		// Converts the chunk containers' origin to chunk coordinates
		glm::vec2 chunkContainerOriginPos = Terrain::worldToChunkCoords(m_chunkContainerOriginWorldPos);

		// Unassign the bottommost chunks from the chunk containers
		for (size_t i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			ChunkContainer& chunkContainer = m_chunkContainers[i];
			chunkContainer.chunk->containerIndex = -1;
			chunkContainer.chunk = nullptr;

			clearWorldPositions(chunkContainer);
			clearUVOffsetIndices(chunkContainer);
		}

		// Reassign the existing chunk containers
		size_t chunkContainerIndex = CHUNK_CONTAINER_DISTANCE + 1;
		for (size_t j = 0; j < CHUNK_CONTAINER_DISTANCE; j++)
		{
			for (size_t i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
			{
				ChunkContainer& chunkContainer = m_chunkContainers[chunkContainerIndex];
				chunkContainer.chunk->containerIndex -= (CHUNK_CONTAINER_DISTANCE + 1);
				m_chunkContainers[chunkContainerIndex - (CHUNK_CONTAINER_DISTANCE + 1)].chunk = chunkContainer.chunk;

				if (chunkContainer.chunk->hasFullyLoaded)
					updateDrawingBuffers(chunkContainerIndex - (CHUNK_CONTAINER_DISTANCE + 1));

				chunkContainerIndex++;
			}
		}

		// Assign the topmost containers, or generate them if necessary
		for (int i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			glm::vec2 chunkPosition = glm::vec2(chunkContainerOriginPos.x + i, chunkContainerOriginPos.y + (CHUNK_CONTAINER_DISTANCE + 1));
			Chunk* chunk = m_terrain->getChunk(chunkPosition);
			if (!chunk)
			{
				chunk = m_terrain->createChunk(chunkPosition);
				chunks.push_back(chunk);
			}

			chunk->containerIndex = (CHUNK_CONTAINER_SIZE - 1) - CHUNK_CONTAINER_DISTANCE + i;

			ChunkContainer& chunkContainer = m_chunkContainers[chunk->containerIndex];
			chunkContainer.chunk = chunk;

			if (chunk->hasFullyLoaded)
				updateDrawingBuffers(chunk->containerIndex);
		}

		// Shift the chunk container origin by 1 chunk
		m_chunkContainerOriginWorldPos.y += CHUNK_SIZE * BLOCK_SIZE;
	}

	// Check if we need to update the chunk containers from the bottom
	if (cameraPosition.y - cameraHeight * 0.5f - CAMERA_VIEW_BUFFER_CONTAINER_REASSIGN <= m_chunkContainerOriginWorldPos.y && cameraPosition.y - cameraHeight * 0.5f >= -TERRAIN_CHUNK_HEIGHT * CHUNK_SIZE * BLOCK_SIZE)
	{
		// Converts the chunk containers' origin to chunk coordinates
		glm::vec2 chunkContainerOriginPos = Terrain::worldToChunkCoords(m_chunkContainerOriginWorldPos);

		// Unassign the topmost chunks from the chunk containers
		for (size_t i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			ChunkContainer& chunkContainer = m_chunkContainers[i + (CHUNK_CONTAINER_SIZE - 1) - CHUNK_CONTAINER_DISTANCE];
			chunkContainer.chunk->containerIndex = -1;
			chunkContainer.chunk = nullptr;

			clearWorldPositions(chunkContainer);
			clearUVOffsetIndices(chunkContainer);
		}

		// Reassign the existing chunk containers
		size_t chunkContainerIndex = (CHUNK_CONTAINER_SIZE - 1) - (CHUNK_CONTAINER_DISTANCE + 1);
		for (size_t j = 0; j < CHUNK_CONTAINER_DISTANCE; j++)
		{
			for (size_t i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
			{
				ChunkContainer& chunkContainer = m_chunkContainers[chunkContainerIndex];
				chunkContainer.chunk->containerIndex += (CHUNK_CONTAINER_DISTANCE + 1);
				m_chunkContainers[chunkContainerIndex + (CHUNK_CONTAINER_DISTANCE + 1)].chunk = chunkContainer.chunk;

				if (chunkContainer.chunk->hasFullyLoaded)
					updateDrawingBuffers(chunkContainerIndex + (CHUNK_CONTAINER_DISTANCE + 1));

				chunkContainerIndex--;
			}
		}

		// Assign the bottommost containers, or generate them if necessary
		for (int i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			glm::vec2 chunkPosition = glm::vec2(chunkContainerOriginPos.x + i, chunkContainerOriginPos.y - 1);
			Chunk* chunk = m_terrain->getChunk(chunkPosition);
			if (!chunk)
			{
				chunk = m_terrain->createChunk(chunkPosition);
				chunks.push_back(chunk);
			}

			chunk->containerIndex = i;

			ChunkContainer& chunkContainer = m_chunkContainers[chunk->containerIndex];
			chunkContainer.chunk = chunk;

			if (chunk->hasFullyLoaded)
				updateDrawingBuffers(chunk->containerIndex);
		}

		// Shift the chunk container origin by 1 chunk
		m_chunkContainerOriginWorldPos.y -= CHUNK_SIZE * BLOCK_SIZE;
	}

	return chunks;
}

void TerrainRenderer::update(const Camera& camera)
{
#ifdef _DEBUG
	if (Input::getInstance()->isKeyPressed(GLFW_KEY_F))
	{
		for (size_t i = 0; i < CHUNK_CONTAINER_SIZE; i++)
		{
			if (m_chunkContainers[i].chunk->hasFullyLoaded)
				updateDrawingBuffers(i);
		}
	}
#endif
}

void TerrainRenderer::updateDrawingBuffers(const size_t containerIndex)
{
	const ChunkContainer& chunkContainer = m_chunkContainers[containerIndex];

	// Cache the blocks pointer and the blockIndexMap pointer
	Block* blocks = chunkContainer.chunk->blocks;
	unsigned int* blockIndexMap = chunkContainer.chunk->blockIndexMap;

	// Collect the container's world positions (in sorted order)
	glm::vec2 worldPositions[CHUNK_SIZE * CHUNK_SIZE];
	for (size_t i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		worldPositions[i] = blocks[blockIndexMap[i]].transform.position;
	}

	// Collect the container's uv offset indices (in sorted order)
	unsigned int uvOffsetIndices[CHUNK_SIZE * CHUNK_SIZE];
	for (size_t i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		uvOffsetIndices[i] = blocks[blockIndexMap[i]].renderable.uvOffsetIndex;
	}

	// Update the container's world positions
	glBindBuffer(GL_ARRAY_BUFFER, chunkContainer.worldPositionsVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2) * CHUNK_SIZE * CHUNK_SIZE, &worldPositions[0][0]);

	// Update the container's uv offset indices
	glBindBuffer(GL_ARRAY_BUFFER, chunkContainer.uvOffsetIndicesVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(unsigned int) * CHUNK_SIZE * CHUNK_SIZE, &uvOffsetIndices[0]);
}

void TerrainRenderer::render(const Camera& camera) const
{
	const glm::mat4& projection = camera.getProjectionMatrix();
	const glm::mat4& view = camera.getViewMatrix();

	AssetManager* assetManager = AssetManager::getInstance();
	unsigned int terrainShaderID = assetManager->getShader("terrainShader");
	unsigned int blockSpritesheet = assetManager->getTexture("blockSpritesheet");

	// Use the shader
	glUseProgram(terrainShaderID);

	// Upload the matrices
	glUniformMatrix4fv(0, 1, GL_FALSE, &projection[0][0]);
	glUniformMatrix4fv(1, 1, GL_FALSE, &view[0][0]);

	// Upload a tint color
	glUniform4f(5, 1.0f, 1.0f, 1.0f, 1.0f);

	// Bind the texture
	glBindTexture(GL_TEXTURE_2D, blockSpritesheet);

	for (size_t i = 0; i < CHUNK_CONTAINER_SIZE; i++)
	{
		if (m_chunkContainers[i].chunk && m_chunkContainers[i].chunk->hasFullyLoaded)
		{
			// Bind the chunk's VAO
			glBindVertexArray(m_chunkContainers[i].vao);

			unsigned int blockCountSum = 0;
			for (size_t j = 1; j < BLOCK_COUNT; j++)
			{
				// Add to the block sum so the instance offset is consistent
				blockCountSum += m_chunkContainers[i].chunk->blockCount[j - 1];

				// Don't render this block type if there aren't any present in the chunk
				if (m_chunkContainers[i].chunk->blockCount[j] == 0) continue;

				// Gets the render data for the current block
				const Renderable& blockRenderData = BlockContainer::getBlockRenderData((BlockType)j);
				const Texture& blockTexture = blockRenderData.texture;
				glm::vec2 blockTileDimensions = blockRenderData.tileDimensions;
				const glm::vec2* blockUVOffsets = blockRenderData.uvOffsets;

				// Upload uniforms
				glm::vec2 uvOffsetScaleFactor = blockTexture.dimensions / (blockTileDimensions - glm::vec2(TEXTURE_SHRINK_FACTOR));
				glUniform2fv(4, 1, &uvOffsetScaleFactor[0]);
				glUniform2fv(6, MAX_ANIMATION_LENGTH, &blockUVOffsets[0][0]);

				// Draw the block using instanced rendering
				glDrawElementsInstancedBaseInstance(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, (void*)0, m_chunkContainers[i].chunk->blockCount[j], blockCountSum);
			}
		}
	}
}

void TerrainRenderer::clearWorldPositions(const ChunkContainer& chunkContainer)
{
	glm::vec2 worldPositions[CHUNK_SIZE * CHUNK_SIZE];
	memset(worldPositions, 0, CHUNK_SIZE * CHUNK_SIZE);

	glBindBuffer(GL_ARRAY_BUFFER, chunkContainer.worldPositionsVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2) * CHUNK_SIZE * CHUNK_SIZE, &worldPositions[0][0]);
}

void TerrainRenderer::clearUVOffsetIndices(const ChunkContainer& chunkContainer)
{
	unsigned int uvOffsetIndices[CHUNK_SIZE * CHUNK_SIZE];
	memset(uvOffsetIndices, 0, CHUNK_SIZE * CHUNK_SIZE);

	glBindBuffer(GL_ARRAY_BUFFER, chunkContainer.uvOffsetIndicesVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(unsigned int) * CHUNK_SIZE * CHUNK_SIZE, &uvOffsetIndices[0]);
}
