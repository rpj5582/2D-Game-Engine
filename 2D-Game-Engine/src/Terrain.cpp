#include "stdafx.h"
#include "Terrain.h"

#include "Renderer.h"
#include "AssetManager.h"

Terrain::Terrain(int seed, const Camera& camera, unsigned int vertexBufferID, unsigned int indexBufferID) : m_camera(camera)
{
	m_seed = seed;

	m_noise = new SimplexNoise(m_seed, 0.25f);
	m_visibleChunks = new Chunk[CHUNK_VIEW_SIZE];

	memset(m_visibleChunks, 0, sizeof(Chunk) * CHUNK_VIEW_SIZE);

	glm::vec2 worldPositions[CHUNK_SIZE * CHUNK_SIZE] = {};
	unsigned int uvOffsetIndices[CHUNK_SIZE * CHUNK_SIZE] = {};

	for (unsigned int i = 0; i < CHUNK_VIEW_SIZE; i++)
	{
		// VAO
		glGenVertexArrays(1, &m_visibleChunks[i].vao);
		glBindVertexArray(m_visibleChunks[i].vao);

		// Vertices and indices
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));

		// World positions
		glGenBuffers(1, &m_visibleChunks[i].worldPositionsVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_visibleChunks[i].worldPositionsVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * CHUNK_SIZE * CHUNK_SIZE, &worldPositions[0][0], GL_STREAM_DRAW);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
		glVertexAttribDivisor(2, 1);

		// UV offset indices
		glGenBuffers(1, &m_visibleChunks[i].uvOffsetIndicesVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_visibleChunks[i].uvOffsetIndicesVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int) * CHUNK_SIZE * CHUNK_SIZE, &uvOffsetIndices[0], GL_STREAM_DRAW);
		
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(unsigned int), (void*)0);
		glVertexAttribDivisor(3, 1);
	}

	m_caveWorm = genCaveWorm({ 0, 0 });
	genStartingChunks(camera.getPosition());
}

Terrain::~Terrain()
{
	for (size_t i = 0; i < CHUNK_VIEW_SIZE; i++)
	{
		glDeleteBuffers(1, &m_visibleChunks[i].worldPositionsVBO);
		glDeleteBuffers(1, &m_visibleChunks[i].uvOffsetIndicesVBO);
		glDeleteVertexArrays(1, &m_visibleChunks[i].vao);
	}

	delete[] m_visibleChunks;
	delete m_noise;
	delete[] m_caveWorm;
}

void Terrain::update()
{
	// Check if the camera has moved outside of the visible chunk range and new chunks should be generated
	checkGenChunks();

	// TEMP - animate grass
	/*for (size_t k = 0; k < CHUNK_VIEW_SIZE; k++)
	{
		Chunk& chunk = m_visibleChunks[k];

		for (size_t j = 0; j < CHUNK_SIZE; j++)
		{
			for (size_t i = 0; i < CHUNK_SIZE; i++)
			{
				if (chunk.blocks[i + j * CHUNK_SIZE].id == GRASS)
				{
					chunk.blocks[i + j * CHUNK_SIZE].renderData.uvOffsetIndex =
						(chunk.blocks[i + j * CHUNK_SIZE].renderData.uvOffsetIndex + 1) % 5;
				}
			}
		}

		updateUVOffsetIndices(&chunk);
	}*/

	// Generate chunks in the queue
	genChunks();
}

std::vector<Chunk*> Terrain::getCollidingChunks(const Sprite& sprite)
{
	glm::vec2 position = sprite.transform.position;

	glm::vec2 minPosition = position - sprite.transform.size / 2.0f;
	glm::vec2 maxPosition = position + sprite.transform.size / 2.0f;

	glm::vec2 minChunkPosition = worldToChunkCoords(minPosition);
	glm::vec2 maxChunkPosition = worldToChunkCoords(maxPosition);

	glm::vec2 chunkOriginPosition = worldToChunkCoords(m_chunkOriginWorldPos);

	std::vector<Chunk*> intersectingChunks;
	for (int j = (int)minChunkPosition.y; j <= maxChunkPosition.y; j++)
	{
		for (int i = (int)minChunkPosition.x; i <= maxChunkPosition.x; i++)
		{
			for (unsigned int k = 0; k < CHUNK_VIEW_SIZE; k++)
			{
				glm::vec2 chunkPos = glm::vec2(chunkOriginPosition.x + k % (CHUNK_VIEW_DISTANCE + 1), chunkOriginPosition.y + k / (CHUNK_VIEW_DISTANCE + 1));
				if (chunkPos.x == i && chunkPos.y == j)
				{
					Chunk* chunk = m_visibleChunkMap[k];
					bool alreadyIntersecting = false;
					for (size_t n = 0; n < intersectingChunks.size(); n++)
					{
						if (intersectingChunks[n] == chunk)
						{
							alreadyIntersecting = true;
							break;
						}
					}

					if(!alreadyIntersecting)
						intersectingChunks.push_back(chunk);

					break;
				}
			}
		}
	}

	return intersectingChunks;
}

Chunk* Terrain::getVisibleChunks() const
{
	return m_visibleChunks;
}

void Terrain::genStartingChunks(glm::vec2 startingPosition)
{
	m_visibleChunkMap.clear();

	glm::vec2 offset = worldToChunkCoords(startingPosition);

	// Calculates the bottom left origin of the visible chunks
	m_chunkOriginWorldPos = chunkToWorldCoords(offset) - glm::vec2(CHUNK_SIZE * BLOCK_SIZE * (CHUNK_VIEW_DISTANCE / 2.0f));

	int initialY = (int)(offset.y + (-CHUNK_VIEW_DISTANCE / 2.0f));
	int initialX = (int)(offset.x + (-CHUNK_VIEW_DISTANCE / 2.0f));

	// Collect the chunk info for each chunk to generate (what its world position is and where to store it in the array)
	std::vector<std::pair<glm::vec2, Chunk*>> chunkInfo;
	unsigned int index = 0;
	for (int y = initialY; y <= CHUNK_VIEW_DISTANCE / 2.0f; y++)
	{
		for (int x = initialX; x <= CHUNK_VIEW_DISTANCE / 2.0f; x++)
		{
			Chunk* chunk = &m_visibleChunks[index];
			glm::vec2 chunkWorldPosition = chunkToWorldCoords(glm::vec2(x, y));

			chunkInfo.push_back({ chunkWorldPosition, chunk });

			m_visibleChunkMap[index] = chunk;
			index++;
		}
	}

	// Adds the chunk info to the queue
	queueGenChunks(chunkInfo);
}

void Terrain::genChunks()
{
	// Take some chunk info from the queue and start a thread to generate the chunk
	if (m_chunkInfo.size() > 0)
	{
		std::pair<glm::vec2, Chunk*> chunkInfo = m_chunkInfo.front();

		// Generate the chunk in its own thread
		m_genChunkThreads.push_back(std::async(std::launch::async, &Terrain::genChunkThreaded, this, chunkInfo.first, chunkInfo.second));
		
		// Remove the chunk info from the queue
		m_chunkInfo.erase(m_chunkInfo.begin());
	}	

	// Check if each thread is done
	for (size_t i = 0; i < m_genChunkThreads.size(); i++)
	{
		if (m_genChunkThreads[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			// If the thread is done, update the world positions and uv offset indices for rendering
			const Chunk* chunk = m_genChunkThreads[i].get();
			updateWorldPositions(chunk);
			updateUVOffsetIndices(chunk);

			// Thread is done, remove it from the list
			m_genChunkThreads.erase(m_genChunkThreads.begin() + i);
		}
	}
}

void Terrain::queueGenChunks(const std::vector<std::pair<glm::vec2, Chunk*>>& chunkInfo)
{
	m_chunkInfo.insert(m_chunkInfo.end(), chunkInfo.begin(), chunkInfo.end());
}

Chunk* Terrain::genChunkThreaded(glm::vec2 chunkWorldPosition, Chunk* chunk)
{
	// Allocate memory to put the generated chunk into
	Block* blocks = new Block[CHUNK_SIZE * CHUNK_SIZE];
	unsigned int blockCount[BLOCK_COUNT] = {};

	// Calculate the surface height values
	int surfaceHeights[CHUNK_SIZE];
	for (size_t i = 0; i < CHUNK_SIZE; i++)
	{
		surfaceHeights[i] = (int)(roundf(m_noise->fractal(SURFACE_OCTAVES, (chunkWorldPosition.x + i * BLOCK_SIZE + 1) / SMOOTHNESS) * HEIGHT_FLUX / BLOCK_SIZE) * BLOCK_SIZE);
	}

	for (size_t j = 0; j < CHUNK_SIZE; j++)
	{
		// Cache the block Y value
		int blockY = (int)(chunkWorldPosition.y + j * BLOCK_SIZE);

		for (size_t i = 0; i < CHUNK_SIZE; i++)
		{
			// Cache the block X value
			int blockX = (int)(chunkWorldPosition.x + i * BLOCK_SIZE);

			size_t blockIndex = i + j * CHUNK_SIZE;

			// Cache the surface height
			int surfaceHeight = surfaceHeights[i];

			// Generate the stone value
			int stoneValue = (int)roundf(m_noise->fractal(STONE_OCTAVES, (blockX + 1) / SMOOTHNESS, (blockY + 1) / SMOOTHNESS) * STONE_FLUX / BLOCK_SIZE) * BLOCK_SIZE;

			// Add a grass block if the we're at the surface value
			if (blockY == surfaceHeight)
			{
				setBlock(blocks[blockIndex], GRASS, chunkWorldPosition + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE), 2);
				blockCount[GRASS]++;
			}
			else if (blockY < surfaceHeight) // Else add a block if the we're below the surface value
			{
				if (blockY < surfaceHeight - 8 * BLOCK_SIZE && stoneValue >= STONE_WEIGHT)
				{
					setBlock(blocks[blockIndex], STONE, chunkWorldPosition + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE), 0);
					blockCount[STONE]++;
				}
				else
				{
					setBlock(blocks[blockIndex], DIRT, chunkWorldPosition + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE), 0);
					blockCount[DIRT]++;
				}
			}
			else // Add an air block if we're above the surface value
			{
				setBlock(blocks[blockIndex], AIR, chunkWorldPosition + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE), 0);
				blockCount[AIR]++;
			}
		}
	}

	bool isAirChunk = false;
	bool isUndergroundChunk = false;

	if (blockCount[AIR] == CHUNK_SIZE * CHUNK_SIZE)
		isAirChunk = true;
	else if (chunkWorldPosition.y + CHUNK_SIZE * BLOCK_SIZE < 0)
		isUndergroundChunk = true;

	ChunkType chunkType = isAirChunk ? CHUNK_AIR : (isUndergroundChunk ? CHUNK_UNDERGROUND : CHUNK_SURFACE);

	// Update grass
	//updateGrassBlocks(chunkWorldPosition, blocks);

	// Generate cave
	genCave(chunkType, blocks, blockCount, chunkWorldPosition);

	// Sort the chunk's blocks
	std::sort(blocks, blocks + CHUNK_SIZE * CHUNK_SIZE, [](const Block& block1, const Block &block2)
	{
		return block1.blockType < block2.blockType;
	});

	// Now that the chunk has been generated, copy it to the appropriate chunk reference
	m_mutex.lock();
	memcpy_s(chunk->blocks, sizeof(Block) * CHUNK_SIZE * CHUNK_SIZE, blocks, sizeof(Block) * CHUNK_SIZE * CHUNK_SIZE);
	memcpy_s(chunk->blockCount, sizeof(unsigned int) * BLOCK_COUNT, blockCount, sizeof(unsigned int) * BLOCK_COUNT);
	chunk->chunkType = chunkType;
	chunk->hasLoaded = true;
	m_mutex.unlock();

	delete[] blocks;
	return chunk;
}

void Terrain::updateGrassBlocks(glm::vec2 chunkWorldPosition, Block* blocks)
{
	for (int j = 0; j < CHUNK_SIZE; j++)
	{
		for (int i = 0; i < CHUNK_SIZE; i++)
		{
			unsigned int blockIndex = i + j * CHUNK_SIZE;
			if (blocks[blockIndex].blockType == GRASS)
			{
				// Check if we should use corner grass pieces
				if (i - 1 >= 0 && blocks[(i - 1) + j * CHUNK_SIZE].blockType == AIR)
				{
					setBlock(blocks[blockIndex], GRASS, chunkWorldPosition + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE), 1);
				}
				else if (i + 1 < CHUNK_SIZE && blocks[(i + 1) + j * CHUNK_SIZE].blockType == AIR)
				{
					setBlock(blocks[blockIndex], GRASS, chunkWorldPosition + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE), 3);
				}
			}
			else if (blocks[blockIndex].blockType == DIRT)
			{
				// Check if we should use side grass pieces
				if (i - 1 >= 0 && blocks[(i - 1) + j * CHUNK_SIZE].blockType == AIR)
				{
					setBlock(blocks[blockIndex], GRASS, chunkWorldPosition + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE), 0);
				}
				else if (i + 1 < CHUNK_SIZE && blocks[(i + 1) + j * CHUNK_SIZE].blockType == AIR)
				{
					setBlock(blocks[blockIndex], GRASS, chunkWorldPosition + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE), 4);
				}
			}
		}
	}
}

void Terrain::genCave(ChunkType chunkType, Block* blocks, unsigned int blockCount[BLOCK_COUNT], glm::vec2 chunkWorldPosition)
{
	/*if (chunkType == CHUNK_UNDERGROUND)
	{
		for (size_t j = 0; j < CHUNK_SIZE; j++)
		{
			int blockY = (int)(chunkWorldPosition.y + j * BLOCK_SIZE);

			for (size_t i = 0; i < CHUNK_SIZE; i++)
			{
				int blockX = (int)(chunkWorldPosition.x + i * BLOCK_SIZE);

				float caveNoise = m_noise->fractal(2, blockX * (1.0f / CHUNK_SIZE / 2.0f), blockY * (1.0f / CHUNK_SIZE / 2.0f));
				if (caveNoise < 0.1f)
				{
					size_t blockIndex = i + CHUNK_SIZE * j;
					blockCount[blocks[blockIndex].blockType]--;
					setBlock(blocks[blockIndex], AIR, blocks[blockIndex].sprite.transform.position, 0);
					blockCount[AIR]++;
				}
			}
		}
	}*/

	for (size_t i = 0; i < m_caveWormLength; i++)
	{
		glm::vec2 caveWormSegment = m_caveWorm[i];
		if (caveWormSegment.x >= chunkWorldPosition.x && caveWormSegment.x < chunkWorldPosition.x + CHUNK_SIZE * BLOCK_SIZE &&
			caveWormSegment.y >= chunkWorldPosition.y && caveWormSegment.y < chunkWorldPosition.y + CHUNK_SIZE * BLOCK_SIZE)
		{
			glm::vec2 chunkPosition = worldToChunkCoords(caveWormSegment);
			glm::vec2 localBlockPosition = caveWormSegment - glm::vec2(chunkPosition.x * CHUNK_SIZE * BLOCK_SIZE, chunkPosition.y * CHUNK_SIZE * BLOCK_SIZE);
			localBlockPosition /= 16.0f;
			size_t blockIndex = (size_t)(localBlockPosition.x + CHUNK_SIZE * localBlockPosition.y);

			blockCount[blocks[blockIndex].blockType]--;
			setBlock(blocks[blockIndex], AIR, blocks[blockIndex].sprite.transform.position, 0);
			blockCount[AIR]++;
		}
		else
			break;
	}
}

glm::vec2* Terrain::genCaveWorm(glm::vec2 startChunkWorldPosition)
{
	float wormMaxLength = SimplexNoise::noise(m_seed + 0.1f / SMOOTHNESS);
	m_caveWormLength = (size_t)(map(wormMaxLength, -1, 1, CAVE_WORM_LENGTH_MIN, CAVE_WORM_LENGTH_MAX));

	glm::vec2* wormPositions = new glm::vec2[(size_t)m_caveWormLength];

	// Chooses a random block within the chunk to start the worm
	//float wormStartX = SimplexNoise::noise(startChunkWorldPosition.x + 0.1f / SMOOTHNESS);
	//float wormStartY = SimplexNoise::noise(startChunkWorldPosition.y + 0.1f / SMOOTHNESS);

	//wormStartX = (float)(int)(map(wormStartX, -1, 1, startChunkWorldPosition.x, startChunkWorldPosition.x + CHUNK_SIZE * BLOCK_SIZE));
	//wormStartY = (float)(int)(map(wormStartY, -1, 1, startChunkWorldPosition.y, startChunkWorldPosition.y + CHUNK_SIZE * BLOCK_SIZE));

	//glm::vec2 wormStart = snapToBlockGrid(glm::vec2(wormStartX, wormStartY)); // Snaps starting world position to grid
	
	/*glm::vec2 wormStart = glm::vec2(-1024.0f);
	
	glm::vec2 wormCurrent = wormStart;

	SimplexNoise noiseX = SimplexNoise((int)(m_seed / 2.0f - wormStart.x), 0.15f, 2.0f);
	SimplexNoise noiseY = SimplexNoise((int)(m_seed / 4.0f + wormStart.y + 384.0f), 0.15f, 2.0f);

	wormPositions[0] = wormStart;
	for (size_t i = 1; i < m_caveWormLength; i++)
	{
		float x = noiseX.fractal(2, wormCurrent.x / SMOOTHNESS, wormCurrent.y / SMOOTHNESS);
		if (x >= 0)
			wormCurrent.x += BLOCK_SIZE;
		else
			wormCurrent.x -= BLOCK_SIZE;

		float y = noiseY.fractal(2, wormCurrent.x / SMOOTHNESS, wormCurrent.y / SMOOTHNESS);
		if (y >= 0)
			wormCurrent.y += BLOCK_SIZE;
		else
			wormCurrent.y -= BLOCK_SIZE;

		wormPositions[i] = wormCurrent;
	}*/

	int wormPoint1 = 0;
	int wormPoint2 = 10;
	for (size_t i = 1; i < m_caveWormLength; i++)
	{
		float noiseValue = m_noise->fractal(2, wormPoint1, wormPoint2);
		if (noiseValue >= 0.25f)
		{
			// Right
		}
		else if (noiseValue >= 0)
		{
			// Left
		}
		else if (noiseValue >= -0.25f)
		{
			// Up
		}
		else
		{
			// Down
		}
	}

	return wormPositions;
}

void Terrain::updateWorldPositions(const Chunk* chunk)
{
	glm::vec2 worldPositions[CHUNK_SIZE * CHUNK_SIZE];
	for (size_t i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		worldPositions[i] = chunk->blocks[i].sprite.transform.position;
	}

	glBindBuffer(GL_ARRAY_BUFFER, chunk->worldPositionsVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2) * CHUNK_SIZE * CHUNK_SIZE, &worldPositions[0][0]);
}

void Terrain::updateUVOffsetIndices(const Chunk* chunk)
{
	unsigned int uvOffsetIndices[CHUNK_SIZE * CHUNK_SIZE];
	for (size_t i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		uvOffsetIndices[i] = chunk->blocks[i].sprite.renderData.uvOffsetIndex;
	}

	glBindBuffer(GL_ARRAY_BUFFER, chunk->uvOffsetIndicesVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(unsigned int) * CHUNK_SIZE * CHUNK_SIZE, &uvOffsetIndices[0]);
}

void Terrain::checkGenChunks()
{
	glm::vec2 cameraPosition = m_camera.getPosition();
	int cameraWidth = m_camera.getWidth();
	int cameraHeight = m_camera.getHeight();

	// Check if we need to load new chunks on the right
	if (cameraPosition.x + cameraWidth * 0.5f + CAMERA_VIEW_BUFFER >= m_chunkOriginWorldPos.x + CHUNK_SIZE * BLOCK_SIZE * (CHUNK_VIEW_DISTANCE + 1))
	{
		// List of chunk info needed to generate the chunks
		std::vector<std::pair<glm::vec2, Chunk*>> chunkInfo;

		// Need to generate chunks on the right and remap the chunk pointers
		std::unordered_map<unsigned int, Chunk*> newVisibleChunkMap;
		for (unsigned int j = 0; j < CHUNK_VIEW_DISTANCE + 1; j++)
		{
			// Start at i = 1 since the leftmost chunks will be overwritten, and should be skipped
			for (unsigned int i = 1; i < CHUNK_VIEW_DISTANCE + 1; i++)
			{
				// Calculate the chunk's index
				unsigned int index = i + j * (CHUNK_VIEW_DISTANCE + 1);

				// Shift over existing chunks
				newVisibleChunkMap[index - 1] = m_visibleChunkMap[index];
			}

			// Now that all other chunks have been shifted, collect info for chunk generation and map it to the rightmost side

			// Calculate index of rightmost chunk for this row
			unsigned int index = CHUNK_VIEW_DISTANCE + j * (CHUNK_VIEW_DISTANCE + 1);

			// Calculate chunk's world position for generation
			glm::vec2 chunkWorldPosition = glm::vec2(
				m_chunkOriginWorldPos.x + CHUNK_SIZE * BLOCK_SIZE * (CHUNK_VIEW_DISTANCE + 1),
				m_chunkOriginWorldPos.y + CHUNK_SIZE * BLOCK_SIZE * j);

			// Get the pointer to the chunk that should be overwritten (the chunk mapped to the leftmost side)
			Chunk* oldChunk = m_visibleChunkMap[index - CHUNK_VIEW_DISTANCE];

			// Store the chunk info for generating the chunks later and store the chunk's pointer in the new map
			chunkInfo.push_back({ chunkWorldPosition, oldChunk });
			newVisibleChunkMap[index] = oldChunk;
		}

		// Save the new chunk map
		m_visibleChunkMap = newVisibleChunkMap;

		// Shift the chunk origin by 1 chunk
		m_chunkOriginWorldPos.x += CHUNK_SIZE * BLOCK_SIZE;

		// Queue the chunk info
		queueGenChunks(chunkInfo);
	}

	// Check if we need to load new chunks on the left
	if (cameraPosition.x - cameraWidth * 0.5f - CAMERA_VIEW_BUFFER <= m_chunkOriginWorldPos.x)
	{
		// List of chunk info needed to generate the chunks
		std::vector<std::pair<glm::vec2, Chunk*>> chunkInfo;

		// Need to generate chunks on the right and remap the chunk pointers
		std::unordered_map<unsigned int, Chunk*> newVisibleChunkMap;
		for (unsigned int j = 0; j < CHUNK_VIEW_DISTANCE + 1; j++)
		{
			// End at i = CHUNK_VIEW_DISTANCE - 1 since the rightmost chunks will be overwritten, and should be skipped
			for (unsigned int i = 0; i < CHUNK_VIEW_DISTANCE; i++)
			{
				// Calculate the chunk's index
				unsigned int index = i + j * (CHUNK_VIEW_DISTANCE + 1);

				// Shift over existing chunks
				newVisibleChunkMap[index + 1] = m_visibleChunkMap[index];
			}

			// Now that all other chunks have been shifted, collect info for chunk generation and map it to the leftmost side

			// Calculate index of leftmost chunk for this row
			unsigned int index = j * (CHUNK_VIEW_DISTANCE + 1);

			// Calculate chunk's world position for generation
			glm::vec2 chunkWorldPosition = glm::vec2(
				m_chunkOriginWorldPos.x - CHUNK_SIZE * BLOCK_SIZE,
				m_chunkOriginWorldPos.y + CHUNK_SIZE * BLOCK_SIZE * j);

			// Get the pointer to the chunk that should be overwritten (the chunk mapped to the rightmost side)
			Chunk* oldChunk = m_visibleChunkMap[index + CHUNK_VIEW_DISTANCE];

			// Store the chunk info for generating the chunks later and store the chunk's pointer in the new map
			chunkInfo.push_back({ chunkWorldPosition, oldChunk });
			newVisibleChunkMap[index] = oldChunk;
		}

		// Save the new chunk map
		m_visibleChunkMap = newVisibleChunkMap;

		// Shift the chunk origin by 1 chunk
		m_chunkOriginWorldPos.x -= CHUNK_SIZE * BLOCK_SIZE;

		// Queue the chunk info
		queueGenChunks(chunkInfo);
	}

	// Check if we need to load new chunks on the top
	if (cameraPosition.y + cameraHeight * 0.5f + CAMERA_VIEW_BUFFER >= m_chunkOriginWorldPos.y + CHUNK_SIZE * BLOCK_SIZE * (CHUNK_VIEW_DISTANCE + 1) && cameraPosition.y + cameraHeight * 0.5f <= TERRAIN_CHUNK_HEIGHT * CHUNK_SIZE * BLOCK_SIZE)
	{
		// List of chunk info needed to generate the chunks
		std::vector<std::pair<glm::vec2, Chunk*>> chunkInfo;

		// Need to generate chunks on the top and remap the chunk pointers
		// Start at j = 1 since the bottommost chunks will be overwritten, and should be skipped
		std::unordered_map<unsigned int, Chunk*> newVisibleChunkMap;
		for (int j = 1; j < CHUNK_VIEW_DISTANCE + 1; j++)
		{
			for (int i = 0; i < CHUNK_VIEW_DISTANCE + 1; i++)
			{
				// Calculate the chunk's index
				unsigned int index = i + j * (CHUNK_VIEW_DISTANCE + 1);

				// Shift over existing chunks
				newVisibleChunkMap[index - (CHUNK_VIEW_DISTANCE + 1)] = m_visibleChunkMap[index];
			}

			// Now that all other chunks have been shifted, collect info for chunk generation and map it to the topmost side

			// Calculate index of topmost chunk for this column
			unsigned int index = j + (CHUNK_VIEW_DISTANCE + 1) * CHUNK_VIEW_DISTANCE;

			// Calculate chunk's world position for generation
			glm::vec2 chunkWorldPosition = glm::vec2(
				m_chunkOriginWorldPos.x + CHUNK_SIZE * BLOCK_SIZE * j,
				m_chunkOriginWorldPos.y + CHUNK_SIZE * BLOCK_SIZE * (CHUNK_VIEW_DISTANCE + 1));

			// Get the pointer to the chunk that should be overwritten (the chunk mapped to the bottommost side)
			Chunk* oldChunk = m_visibleChunkMap[j];

			// Store the chunk info for generating the chunks later and store the chunk's pointer in the new map
			chunkInfo.push_back({ chunkWorldPosition, oldChunk });
			newVisibleChunkMap[index] = oldChunk;
		}

		// Now only chunk at the top left needs to be generated

		// Calculate chunk's world position for generation
		glm::vec2 chunkWorldPosition = glm::vec2(
			m_chunkOriginWorldPos.x,
			m_chunkOriginWorldPos.y + CHUNK_SIZE * BLOCK_SIZE * (CHUNK_VIEW_DISTANCE + 1));

		// Get the pointer to the chunk that should be overwritten (the chunk mapped to the bottommost side)
		Chunk* oldChunk = m_visibleChunkMap[0];

		// Store the chunk info for generating the chunks later and store the chunk's pointer in the new map
		chunkInfo.push_back({ chunkWorldPosition, oldChunk });
		newVisibleChunkMap[(CHUNK_VIEW_DISTANCE + 1) * CHUNK_VIEW_DISTANCE] = oldChunk;

		// Save the new chunk map
		m_visibleChunkMap = newVisibleChunkMap;

		// Shift the chunk origin by 1 chunk
		m_chunkOriginWorldPos.y += CHUNK_SIZE * BLOCK_SIZE;

		// Queue the chunk info
		queueGenChunks(chunkInfo);
	}

	// Check if we need to load new chunks on the bottom
	if (cameraPosition.y - cameraHeight * 0.5f - CAMERA_VIEW_BUFFER <= m_chunkOriginWorldPos.y && cameraPosition.y - cameraHeight * 0.5f >= -TERRAIN_CHUNK_HEIGHT * CHUNK_SIZE * BLOCK_SIZE)
	{
		// List of chunk info needed to generate the chunks
		std::vector<std::pair<glm::vec2, Chunk*>> chunkInfo;

		// Need to generate chunks on the bottom and remap the chunk pointers
		// Start at j = CHUNK_VIEW_DISTANCE and go backwards since the topmost chunks will be overwritten, and should be skipped
		std::unordered_map<unsigned int, Chunk*> newVisibleChunkMap;
		for (int j = CHUNK_VIEW_DISTANCE; j > 0; j--)
		{
			for (int i = 0; i < CHUNK_VIEW_DISTANCE + 1; i++)
			{
				// Calculate the chunk's index
				unsigned int index = i + j * (CHUNK_VIEW_DISTANCE + 1);

				// Shift over existing chunks
				newVisibleChunkMap[index] = m_visibleChunkMap[index - (CHUNK_VIEW_DISTANCE + 1)];
			}

			// Now that all other chunks have been shifted, collect info for chunk generation and map it to the bottommost side

			// Calculate index of bottommost chunk for this column
			unsigned int index = j;

			// Calculate chunk's world position for generation
			glm::vec2 chunkWorldPosition = glm::vec2(
				m_chunkOriginWorldPos.x + CHUNK_SIZE * BLOCK_SIZE * j,
				m_chunkOriginWorldPos.y - CHUNK_SIZE * BLOCK_SIZE);

			// Get the pointer to the chunk that should be overwritten (the chunk mapped to the topmost side)
			Chunk* oldChunk = m_visibleChunkMap[index + (CHUNK_VIEW_DISTANCE + 1) * CHUNK_VIEW_DISTANCE];

			// Store the chunk info for generating the chunks later and store the chunk's pointer in the new map
			chunkInfo.push_back({ chunkWorldPosition, oldChunk });
			newVisibleChunkMap[index] = oldChunk;
		}

		// Now only chunk at the bottom left needs to be generated

		// Calculate chunk's world position for generation
		glm::vec2 chunkWorldPosition = glm::vec2(
			m_chunkOriginWorldPos.x,
			m_chunkOriginWorldPos.y - CHUNK_SIZE * BLOCK_SIZE);

		// Get the pointer to the chunk that should be overwritten (the chunk mapped to the topmost side)
		Chunk* oldChunk = m_visibleChunkMap[(CHUNK_VIEW_DISTANCE + 1) * CHUNK_VIEW_DISTANCE];

		// Store the chunk info for generating the chunks later and store the chunk's pointer in the new map
		chunkInfo.push_back({ chunkWorldPosition, oldChunk });
		newVisibleChunkMap[0] = oldChunk;

		// Save the new chunk map
		m_visibleChunkMap = newVisibleChunkMap;

		// Shift the chunk origin by 1 chunk
		m_chunkOriginWorldPos.y -= CHUNK_SIZE * BLOCK_SIZE;

		// Queue the chunk info
		queueGenChunks(chunkInfo);
	}
}

void Terrain::setBlock(Block& block, BlockType type, glm::vec2 position, unsigned int uvOffsetIndex)
{
	const RenderData& blockRenderData = BlockContainer::getBlockRenderData(type);
	block.blockType = type;
	block.sprite.renderData = blockRenderData;
	block.sprite.renderData.uvOffsetIndex = uvOffsetIndex;
	block.sprite.transform = Transform(position, glm::vec2(BLOCK_SIZE));
}

glm::vec2 Terrain::worldToChunkCoords(glm::vec2 worldPosition) const
{
	float roundFactor = CHUNK_SIZE * BLOCK_SIZE;
	float x = (floorf(worldPosition.x / roundFactor));
	float y = (floorf(worldPosition.y / roundFactor));

	return glm::vec2(x, y);
}

glm::vec2 Terrain::chunkToWorldCoords(glm::vec2 chunkPosition) const
{
	return chunkPosition * (float)(CHUNK_SIZE * BLOCK_SIZE);
}

glm::vec2 Terrain::snapToBlockGrid(glm::vec2 worldPosition) const
{
	return glm::vec2((int)worldPosition.x & 0xfff0, (int)worldPosition.y & 0xfff0);
}
