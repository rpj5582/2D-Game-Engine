#include "stdafx.h"
#include "Terrain.h"

#include "Renderer.h"
#include "AssetManager.h"

#include <algorithm>

Terrain::Terrain(unsigned int vertexBufferID, unsigned int indexBufferID)
{
	m_noise = new SimplexNoise(0.25f);
	m_visibleChunks = new Chunk[CHUNK_VIEW_SIZE];

	memset(m_visibleChunks, 0, sizeof(Chunk) * CHUNK_VIEW_SIZE);

	glm::mat4 worldMatrices[CHUNK_SIZE * CHUNK_SIZE] = {};

	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));

	for (unsigned int i = 0; i < CHUNK_VIEW_SIZE; i++)
	{
		unsigned int vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4) * CHUNK_SIZE * CHUNK_SIZE, &worldMatrices[0][0], GL_STREAM_DRAW);

		glEnableVertexAttribArray(2 + i * 4);
		glEnableVertexAttribArray(3 + i * 4);
		glEnableVertexAttribArray(4 + i * 4);
		glEnableVertexAttribArray(5 + i * 4);

		glVertexAttribPointer(2 + i * 4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
		glVertexAttribPointer(3 + i * 4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)sizeof(glm::vec4));
		glVertexAttribPointer(4 + i * 4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * 2));
		glVertexAttribPointer(5 + i * 4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * 3));

		glVertexAttribDivisor(2 + i * 4, 1);
		glVertexAttribDivisor(3 + i * 4, 1);
		glVertexAttribDivisor(4 + i * 4, 1);
		glVertexAttribDivisor(5 + i * 4, 1);

		m_visibleChunks[i].vbo = vbo;
	}
}

Terrain::~Terrain()
{
	for (size_t i = 0; i < CHUNK_VIEW_SIZE; i++)
	{
		glDeleteBuffers(1, &m_visibleChunks[i].vbo);
	}

	glDeleteVertexArrays(1, &m_vao);

	delete[] m_visibleChunks;
	delete m_noise;
}

unsigned int Terrain::getVAO() const
{
	return m_vao;
}

const Chunk* Terrain::getVisibleChunks() const
{
	return m_visibleChunks;
}

void Terrain::genStartingChunks(glm::vec2 startingPosition)
{
	ptrdiff_t offsetX, offsetY;
	worldToChunkCoords(startingPosition, &offsetX, &offsetY);

	ptrdiff_t initialX = offsetX + (ptrdiff_t)(-CHUNK_VIEW_DISTANCE / 2.0f);
	ptrdiff_t initialY = offsetY + (ptrdiff_t)(-CHUNK_VIEW_DISTANCE / 2.0f);

	size_t index = 0;
	for (ptrdiff_t y = initialY; y <= CHUNK_VIEW_DISTANCE / 2.0f; y++)
	{
		for (ptrdiff_t x = initialX; x <= CHUNK_VIEW_DISTANCE / 2.0f; x++)
		{
			genChunk(x, y, index);
			index++;
		}
	}
}

void Terrain::genChunk(ptrdiff_t x, ptrdiff_t y, size_t index)
{
	assert(index >= 0 && index < CHUNK_VIEW_SIZE);

	Chunk& chunk = m_visibleChunks[index];

	glm::vec2 chunkOriginWorld = chunkToWorldCoords(x, y);

	// Generate the surface height from this chunk's x coordinates
	int surface[CHUNK_SIZE];
	for (size_t i = 0; i < CHUNK_SIZE; i++)
	{
		surface[i] = (int)(roundf(m_noise->fractal(OCTAVES, (chunkOriginWorld.x + i * BLOCK_SIZE + 1) / SMOOTHNESS) * HEIGHT_FLUX / BLOCK_SIZE) * BLOCK_SIZE);
	}

	// Populate the chunk
	for (size_t j = 0; j < CHUNK_SIZE; j++)
	{
		for (size_t i = 0; i < CHUNK_SIZE; i++)
		{
			// Add a dirt block if the we're at or below the surface value
			if (chunkOriginWorld.y + j * BLOCK_SIZE <= surface[i])
			{
				setBlock(chunk.sprites[i + j * CHUNK_SIZE], DIRT, chunkOriginWorld + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE));
				chunk.blockCount[DIRT]++;
			}
			else // Add an air block if we're above the surface value
			{
				setBlock(chunk.sprites[i + j * CHUNK_SIZE], AIR, chunkOriginWorld + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE));
				chunk.blockCount[AIR]++;
			}
		}
	}

	// Sort the chunk's blocks
	std::sort(std::begin(chunk.sprites), std::end(chunk.sprites), [](const Sprite& sprite1, const Sprite& sprite2)
	{
		return sprite1.renderData.textureID < sprite2.renderData.textureID;
	});

	glm::mat4 worldMatrices[CHUNK_SIZE * CHUNK_SIZE];

	// Update the world matrices for each block
	for (size_t i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		glm::mat4 worldMatrix = glm::translate(glm::mat4(), glm::vec3(chunk.sprites[i].transform.position, 0.0f));
		worldMatrix = glm::scale(worldMatrix, glm::vec3(BLOCK_SIZE, BLOCK_SIZE, 0.0f));
		worldMatrices[i] = worldMatrix;
	}

	glBindBuffer(GL_ARRAY_BUFFER, chunk.vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::mat4) * CHUNK_SIZE * CHUNK_SIZE, &worldMatrices[0][0]);
}

void Terrain::setBlock(Sprite& sprite, Blocks type, glm::vec2 position)
{
	const RenderData& block = BlockContainer::getBlock(type);
	sprite.renderData = block;
	sprite.transform.position = position;
	sprite.transform.size = glm::vec2(BLOCK_SIZE);
}

void Terrain::worldToChunkCoords(glm::vec2 worldPosition, ptrdiff_t* x, ptrdiff_t* y) const
{
	assert(x != nullptr && y != nullptr);

	float roundFactor = CHUNK_SIZE * BLOCK_SIZE;
	*x = (int)(roundf(worldPosition.x / roundFactor) * roundFactor);
	*y = (int)(roundf(worldPosition.y / roundFactor) * roundFactor);
}

glm::vec2 Terrain::chunkToWorldCoords(ptrdiff_t x, ptrdiff_t y) const
{
	return glm::vec2(x * CHUNK_SIZE * BLOCK_SIZE, y * CHUNK_SIZE * BLOCK_SIZE);
}
