#include "stdafx.h"
#include "Terrain.h"

#include "Renderer.h"
#include "AssetManager.h"

#define _USE_MATH_DEFINES
#include <math.h>  

#include "Input.h"

Terrain::Terrain(const Camera& camera, unsigned int vertexBufferID, unsigned int indexBufferID) : m_camera(camera)
{
	m_terrainNoise = new SimplexNoise(0.25f);
	m_treeNoise = new SimplexNoise(4.0f, 0.25f);

	memset(m_chunkContainers, 0, sizeof(ChunkContainer) * CHUNK_CONTAINER_SIZE);

	glm::vec2 worldPositions[CHUNK_SIZE * CHUNK_SIZE] = {};
	unsigned int uvOffsetIndices[CHUNK_SIZE * CHUNK_SIZE] = {};

	for (unsigned int i = 0; i < CHUNK_CONTAINER_SIZE; i++)
	{
		// VAO
		glGenVertexArrays(1, &m_chunkContainers[i].vao);
		glBindVertexArray(m_chunkContainers[i].vao);

		// Vertices and indices
		glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 2));

		// World positions
		glGenBuffers(1, &m_chunkContainers[i].worldPositionsVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_chunkContainers[i].worldPositionsVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * CHUNK_SIZE * CHUNK_SIZE, &worldPositions[0][0], GL_STREAM_DRAW);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
		glVertexAttribDivisor(2, 1);

		// UV offset indices
		glGenBuffers(1, &m_chunkContainers[i].uvOffsetIndicesVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_chunkContainers[i].uvOffsetIndicesVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int) * CHUNK_SIZE * CHUNK_SIZE, &uvOffsetIndices[0], GL_STREAM_DRAW);
		
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 1, GL_UNSIGNED_INT, sizeof(unsigned int), (void*)0);
		glVertexAttribDivisor(3, 1);
	}

	genStartingChunks(camera.getPosition());
}

Terrain::~Terrain()
{
	for (size_t i = 0; i < CHUNK_CONTAINER_SIZE; i++)
	{
		glDeleteBuffers(1, &m_chunkContainers[i].worldPositionsVBO);
		glDeleteBuffers(1, &m_chunkContainers[i].uvOffsetIndicesVBO);
		glDeleteVertexArrays(1, &m_chunkContainers[i].vao);
	}

	// Waits for the threads to finish
	for (size_t i = 0; i < m_genChunkThreads.size(); i++)
	{
		if (m_genChunkThreads[i].valid())
			m_genChunkThreads[i].wait();
	}

	for (size_t i = 0; i < m_postGenThreads.size(); i++)
	{
		if (m_postGenThreads[i].valid())
			m_postGenThreads[i].wait();
	}

	m_genChunkThreads.clear();
	m_postGenThreads.clear();

	m_queuedChunksToGen.clear();

	unloadChunks();

	delete m_terrainNoise;
	delete m_treeNoise;
}

void Terrain::update()
{
	// Check if the camera has moved outside of the visible chunk range and the chunk containers should be shifted
	checkShiftChunkContainers();

	// Check if chunks should be generated
	checkGenChunks();

	// Check for chunks that are too far away from the player and should be unloaded
	checkUnloadChunks();

	// TEMP - animate grass
	/*for (size_t k = 0; k < CHUNK_VIEW_SIZE; k++)
	{
		Chunk& chunk = m_chunkContainers[k];

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

	// Check if any threads have finished and remove them
	checkThreadsFinished();

	if (Input::getInstance()->isKeyPressed(GLFW_KEY_R))
	{
		genStartingChunks(m_camera.getPosition());
	}

	if (Input::getInstance()->isKeyPressed(GLFW_KEY_F))
	{
		for (size_t i = 0; i < CHUNK_CONTAINER_SIZE; i++)
		{
			if (m_chunkContainers[i].chunk->hasFullyLoaded)
				updateDrawingBuffers(i);
		}
	}		
}

//std::vector<ChunkContainer*> Terrain::getCollidingChunks(const Sprite& sprite)
//{
//	glm::vec2 position = sprite.transform.position;
//
//	glm::vec2 minPosition = position - sprite.transform.size / 2.0f;
//	glm::vec2 maxPosition = position + sprite.transform.size / 2.0f;
//
//	glm::vec2 minChunkPosition = worldToChunkCoords(minPosition);
//	glm::vec2 maxChunkPosition = worldToChunkCoords(maxPosition);
//
//	glm::vec2 chunkOriginPosition = worldToChunkCoords(m_chunkOriginWorldPos);
//
//	std::vector<ChunkContainer*> intersectingChunks;
//	for (int j = (int)minChunkPosition.y; j <= maxChunkPosition.y; j++)
//	{
//		for (int i = (int)minChunkPosition.x; i <= maxChunkPosition.x; i++)
//		{
//			for (unsigned int k = 0; k < CHUNK_CONTAINER_SIZE; k++)
//			{
//				glm::vec2 chunkPos = glm::vec2(chunkOriginPosition.x + k % (CHUNK_CONTAINER_DISTANCE + 1), chunkOriginPosition.y + k / (CHUNK_CONTAINER_DISTANCE + 1));
//				if (chunkPos.x == i && chunkPos.y == j)
//				{
//					ChunkContainer* chunk = m_chunkContainerMap[k];
//					bool alreadyIntersecting = false;
//					for (size_t n = 0; n < intersectingChunks.size(); n++)
//					{
//						if (intersectingChunks[n] == chunk)
//						{
//							alreadyIntersecting = true;
//							break;
//						}
//					}
//
//					if(!alreadyIntersecting)
//						intersectingChunks.push_back(chunk);
//
//					break;
//				}
//			}
//		}
//	}
//
//	return intersectingChunks;
//}

const ChunkContainer* Terrain::getChunkContainers() const
{
	return m_chunkContainers;
}

Chunk* Terrain::createChunk(glm::vec2 chunkPosition)
{
	if (m_chunks.find(chunkPosition) == m_chunks.end())
	{
		Chunk* chunk = new Chunk(chunkPosition);
		m_chunks[chunk->chunkPosition] = chunk;

		memset(chunk->blocks, 0, CHUNK_SIZE * CHUNK_SIZE);
		memset(chunk->blockCount, 0, BLOCK_COUNT);

		chunk->chunkType = CHUNK_AIR;
		chunk->containerIndex = -1;
		chunk->hasWormHead = false;
		chunk->hasGenerated = false;
		chunk->hasFullyLoaded = false;

		return chunk;
	}
	else
	{
		Output::error("ERROR: Tried to create chunk at X: " + std::to_string(chunkPosition.x) + ", Y: " + std::to_string(chunkPosition.y) + " but a chunk already exists there.");
		return nullptr;
	}
}

void Terrain::genStartingChunks(glm::vec2 startingPosition)
{
	// Deletes any old chunks for a fresh start
	unloadChunks();

	glm::vec2 offset = worldToChunkCoords(startingPosition);

	// Calculates the bottom left origin of the chunk containers
	m_chunkContainerOriginWorldPos = chunkToWorldCoords(offset) - glm::vec2(CHUNK_SIZE * BLOCK_SIZE * (CHUNK_CONTAINER_DISTANCE / 2.0f));

	int initialY = (int)(offset.y + (-CHUNK_CONTAINER_DISTANCE / 2.0f));
	int initialX = (int)(offset.x + (-CHUNK_CONTAINER_DISTANCE / 2.0f));

	// Create empty chunks that need to be generated
	std::vector<Chunk*> startingChunks;
	unsigned int index = 0;
	for (int y = initialY; y < initialY + (CHUNK_CONTAINER_DISTANCE + 1); y++)
	{
		for (int x = initialX; x < initialX + (CHUNK_CONTAINER_DISTANCE + 1); x++)
		{
			Chunk* chunk = createChunk(glm::vec2(x, y));
			chunk->containerIndex = index;

			ChunkContainer& chunkContainer = m_chunkContainers[index];
			chunkContainer.chunk = chunk;

			startingChunks.push_back(chunk);
			index++;
		}
	}

	// Adds the chunk info to the queue
	queueGenChunks(startingChunks);
}

void Terrain::genChunks()
{
	{
		std::unique_lock<std::mutex> lock(m_genQueueMutex);

		// Take some chunk info from the queue and start a thread to generate the chunk
		if (m_queuedChunksToGen.size() > 0)
		{
			Chunk* chunk = m_queuedChunksToGen.front();

			// Generate the chunk in its own thread
			m_genChunkThreads.push_back(std::async(std::launch::async, &Terrain::genChunkThreaded, this, chunk));

			// Remove the chunk info from the queue
			m_queuedChunksToGen.erase(m_queuedChunksToGen.begin());
		}
	}
}

void Terrain::queueGenChunk(Chunk* chunk)
{
	std::unique_lock<std::mutex> lock(m_genQueueMutex);
	m_queuedChunksToGen.push_back(chunk);
}

void Terrain::queueGenChunks(const std::vector<Chunk*>& chunks)
{
	std::unique_lock<std::mutex> lock(m_genQueueMutex);
	m_queuedChunksToGen.insert(m_queuedChunksToGen.end(), chunks.begin(), chunks.end());
}

Chunk* Terrain::genChunkThreaded(Chunk* chunk)
{
	glm::vec2 chunkWorldPosition = chunkToWorldCoords(chunk->chunkPosition);

	// Allocate memory to put the generated chunk into
	Block* blocks = new Block[CHUNK_SIZE * CHUNK_SIZE];
	unsigned int blockCount[BLOCK_COUNT] = {};

	// Allocate memory for the block index map
	unsigned int blockIndexMap[CHUNK_SIZE * CHUNK_SIZE];

	
	int surfaceHeights[CHUNK_SIZE];
	for (size_t i = 0; i < CHUNK_SIZE; i++)
	{
		// Calculate the surface height values
		surfaceHeights[i] = calculateSurfaceHeight(chunkWorldPosition.x, i);
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

			blockIndexMap[blockIndex] = (unsigned int)blockIndex;

			// Cache the surface height
			int surfaceHeight = surfaceHeights[i];

			// Generate the stone value
			int stoneValue = (int)roundf(m_terrainNoise->fractal(STONE_OCTAVES, (blockX + 1) / SMOOTHNESS, (blockY + 1) / SMOOTHNESS) * STONE_FLUX / BLOCK_SIZE) * BLOCK_SIZE;

			// Add a grass block if the we're at the surface value
			if (blockY == surfaceHeight)
			{
				setBlock(blocks[blockIndex], GRASS, chunkWorldPosition + glm::vec2(i * BLOCK_SIZE, j * BLOCK_SIZE), 3);
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

	ChunkType chunkType;
	if (isAirChunk)
	{
		chunkType = CHUNK_AIR;
	}
	else if (isUndergroundChunk)
	{
		chunkType = CHUNK_UNDERGROUND;

		// Generate cave
		genCave(blocks, blockCount, chunkWorldPosition);
	}
	else
	{
		chunkType = CHUNK_SURFACE;

		// Update grass
		//updateGrassBlocks(chunkWorldPosition, blocks);
	}

	// Sort the chunk's block index map so that we can keep the blocks unsorted for later modification,
	// but still be able to copy them to the drawing buffers in sorted way.
	sortBlockIndexMap(blocks, blockIndexMap);

	float noiseValue = SimplexNoise::noise(chunkWorldPosition.x / SMOOTHNESS, chunkWorldPosition.y / SMOOTHNESS);

	// Now that the chunk has been generated, copy it to the appropriate chunk reference
	chunk->mutex.lock();

	// Check to see if this chunk should have a cave worm head for cave entrance generation
	if (chunkType == CHUNK_SURFACE)
	{
		if (abs(noiseValue) > 0.75f)
			chunk->hasWormHead = true;
	}
	
	memcpy_s(chunk->blocks, sizeof(Block) * CHUNK_SIZE * CHUNK_SIZE, blocks, sizeof(Block) * CHUNK_SIZE * CHUNK_SIZE);
	memcpy_s(chunk->blockIndexMap, sizeof(unsigned int) * CHUNK_SIZE * CHUNK_SIZE, blockIndexMap, sizeof(unsigned int) * CHUNK_SIZE * CHUNK_SIZE);
	memcpy_s(chunk->blockCount, sizeof(unsigned int) * BLOCK_COUNT, blockCount, sizeof(unsigned int) * BLOCK_COUNT);
	chunk->chunkType = chunkType;
	chunk->hasGenerated = true;
	chunk->mutex.unlock();

	chunk->cv.notify_all();

	delete[] blocks;
	return chunk;
}

std::vector<Chunk*> Terrain::postGenChunkThreaded(Chunk* chunk)
{
	// The original chunk needs to be included in the list of modified chunks, even if it wasn't modified,
	// so that it can be marked as fully generated later
	std::vector<Chunk*> modifiedChunks{chunk};

	// Check if the chunk should generate a cave worm
	if (chunk->hasWormHead)
	{
		std::vector<Chunk*> caveWormModifiedChunks = genCaveWorm(chunk->chunkPosition);
		modifiedChunks.insert(modifiedChunks.end(), caveWormModifiedChunks.begin(), caveWormModifiedChunks.end());
	}

	// Check to see if we should generate trees
	if (chunk->chunkType == CHUNK_SURFACE)
	{
		std::vector<Chunk*> treeModifiedChunks = genTrees(chunk);
		modifiedChunks.insert(modifiedChunks.end(), treeModifiedChunks.begin(), treeModifiedChunks.end());
	}

	// Remove duplicate modified chunks
	std::sort(modifiedChunks.begin(), modifiedChunks.end());
	modifiedChunks.erase(std::unique(modifiedChunks.begin(), modifiedChunks.end()), modifiedChunks.end());

	return modifiedChunks;
}

void Terrain::unloadChunks()
{
	for (auto it = m_chunks.begin(); it != m_chunks.end(); it++)
	{
		delete it->second;
	}

	m_chunks.clear();
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

void Terrain::genCave(Block* blocks, unsigned int blockCount[BLOCK_COUNT], glm::vec2 chunkWorldPosition)
{
	for (size_t j = 0; j < CHUNK_SIZE; j++)
	{
		int blockY = (int)(chunkWorldPosition.y + j * BLOCK_SIZE);

		for (size_t i = 0; i < CHUNK_SIZE; i++)
		{
			int blockX = (int)(chunkWorldPosition.x + i * BLOCK_SIZE);

			float caveNoise = (m_terrainNoise->fractal(2, blockX * (1.0f / CHUNK_SIZE / 2.0f), blockY * (1.0f / CHUNK_SIZE / 2.0f)));
			float caveCutoff = 1 - abs(chunkWorldPosition.y * 2 / (TERRAIN_CHUNK_HEIGHT * CHUNK_SIZE * BLOCK_SIZE)) - 0.2f;
			caveCutoff = fmaxf(caveCutoff, -0.25f);
			if (caveNoise > caveCutoff)
			{
				size_t blockIndex = i + CHUNK_SIZE * j;
				blockCount[blocks[blockIndex].blockType]--;
				setBlock(blocks[blockIndex], AIR, blocks[blockIndex].sprite.transform.position, 0);
				blockCount[AIR]++;
			}
		}
	}
}

std::vector<Chunk*> Terrain::genCaveWorm(glm::vec2 chunkPosition)
{
	std::vector<Chunk*> modifiedChunks;

	glm::vec2 chunkWorldPosition = chunkToWorldCoords(chunkPosition);

	// Create the worm and set its starting point
	CaveWorm worm;
	worm.headNoisePosition.x = SimplexNoise::noise(chunkWorldPosition.x / SMOOTHNESS);
	worm.headNoisePosition.y = SimplexNoise::noise(chunkWorldPosition.y / SMOOTHNESS);
	worm.headChunkPosition = chunkPosition;

	// Set the worms max length
	float wormNoiseMaxLength = SimplexNoise::noise(chunkPosition.x, chunkPosition.y);
	worm.length = (size_t)roundf(map(wormNoiseMaxLength, -1, 1, CAVE_WORM_LENGTH_MIN, CAVE_WORM_LENGTH_MAX));

	// Chooses a random block within the chunk to start the worm
	float wormStartX = SimplexNoise::noise(chunkWorldPosition.x + 0.1f / SMOOTHNESS, chunkWorldPosition.y + 0.1f / SMOOTHNESS, 0.16f);
	float wormStartY = SimplexNoise::noise(chunkWorldPosition.x + 0.1f / SMOOTHNESS, chunkWorldPosition.y + 0.1f / SMOOTHNESS, 0.64f);

	wormStartX = (float)(int)(map(wormStartX, -1, 1, chunkWorldPosition.x, chunkWorldPosition.x + CHUNK_SIZE * BLOCK_SIZE));
	wormStartY = (float)(int)(map(wormStartY, -1, 1, chunkWorldPosition.y, chunkWorldPosition.y + CHUNK_SIZE * BLOCK_SIZE));

	// Snaps starting world position to grid
	glm::vec2 wormStartPosition = snapToBlockGrid(glm::vec2(wormStartX, wormStartY));
	glm::vec2 wormCurrentPosition = wormStartPosition;

	glm::vec3 currentNoisePosition = worm.headNoisePosition;

	Chunk* chunk = nullptr;
	for (size_t i = 0; i < worm.length; i++)
	{
		// Gets the chunk that the current position is in (might be different)
		glm::vec2 currentChunkPosition = worldToChunkCoords(wormCurrentPosition);
		auto chunkIt = m_chunks.find(currentChunkPosition);
		if (chunkIt == m_chunks.end())
		{			
			// Need to wait and generate the chunk for the worm to continue through
			chunk = createChunk(currentChunkPosition);
			modifiedChunks.push_back(chunk);

			queueGenChunk(chunk);
			
			std::unique_lock<std::mutex> lock(chunk->mutex);
			chunk->cv.wait(lock); // Waits for the chunk to be fully generated
		}
		else
		{
			// Check if the chunk the worm is in has changed
			if (chunk != chunkIt->second)
			{
				chunk = chunkIt->second;
				if (std::find(modifiedChunks.begin(), modifiedChunks.end(), chunk) == modifiedChunks.end())
				{
					modifiedChunks.push_back(chunk);
				}
			}

			std::unique_lock<std::mutex> lock(chunk->mutex);
			if (!chunk->hasGenerated) // Chunk exists, but hasn't fully generated yet
			{
				chunk->cv.wait(lock); // Waits for the chunk to be fully generated
			}
		}

		float wormWidthNoise = SimplexNoise::noise((float)i / worm.length / SMOOTHNESS, currentNoisePosition.y / SMOOTHNESS);
		int wormRadius = (int)roundf(map(wormWidthNoise, -1, 1, CAVE_WORM_RADIUS_MIN, CAVE_WORM_RADIUS_MAX));

		// Convert the current position to block coordinates
		glm::vec2 currentChunkWorldPosition = chunkToWorldCoords(currentChunkPosition);
		glm::vec2 blockWorldDelta = wormCurrentPosition - currentChunkWorldPosition;
		glm::vec2 blockIndices = (glm::vec2((int)(blockWorldDelta.x / BLOCK_SIZE) % CHUNK_SIZE, (int)(blockWorldDelta.y / BLOCK_SIZE) % CHUNK_SIZE));

		{
			// Take ownership of the chunk while carving out the worm
			std::unique_lock<std::mutex> lock(chunk->mutex);

			for (ptrdiff_t j = -wormRadius; j <= wormRadius; j++)
			{
				for (ptrdiff_t k = -wormRadius; k <= wormRadius; k++)
				{
					glm::vec2 currentBlockIndices = blockIndices + glm::vec2(k, j);
					
					// Check if these indices are within the worm radius
					if ((currentBlockIndices.x - blockIndices.x) * (currentBlockIndices.x - blockIndices.x) +
						(currentBlockIndices.y - blockIndices.y) * (currentBlockIndices.y - blockIndices.y) <= wormRadius * wormRadius)
					{
						// Skip this block if it's not within the chunk
						if (currentBlockIndices.x < 0 || currentBlockIndices.x >= CHUNK_SIZE ||
							currentBlockIndices.y < 0 || currentBlockIndices.y >= CHUNK_SIZE) continue;

						Block& block = chunk->blocks[(int)currentBlockIndices.x + (int)currentBlockIndices.y * CHUNK_SIZE];

						chunk->blockCount[block.blockType]--;
						setBlock(block, AIR, block.sprite.transform.position, 0);
						chunk->blockCount[AIR]++;
					}
				}
			}
		}

		// Get the next worm position
		currentNoisePosition.x = worm.headNoisePosition.x + (i * 0.16f);
		currentNoisePosition.y = worm.headNoisePosition.y + (i * 0.64f);

		float noiseValue = SimplexNoise::noise(currentNoisePosition.x, currentNoisePosition.y, currentNoisePosition.z);
		glm::vec2 nextPosition;
		if (noiseValue <= -0.25f)
		{
			nextPosition = glm::vec2(0, -16);
		}
		else if (noiseValue > -0.25f && noiseValue <= 0.20f)
		{
			nextPosition = glm::vec2(16, 0);
		}
		else if (noiseValue > 0.20f && noiseValue <= 0.75f)
		{
			nextPosition = glm::vec2(-16, 0);
		}
		else
		{
			nextPosition = glm::vec2(0, 16);
		}

		wormCurrentPosition += nextPosition;
	}

	return modifiedChunks;
}

std::vector<Chunk*> Terrain::genTrees(Chunk* baseChunk)
{
	std::vector<Chunk*> modifiedChunks;

	glm::vec2 baseChunkWorldPosition = chunkToWorldCoords(baseChunk->chunkPosition);

	for (int i = 0; i < CHUNK_SIZE; i++)
	{
		float treeNoiseValue = m_treeNoise->fractal(TREE_OCTAVES, (baseChunkWorldPosition.x + i * BLOCK_SIZE + 1) / TREE_SMOOTHNESS);
		if (treeNoiseValue > 0.15f)
		{
			// Calculate the surface height again since we don't have it anymore
			int surfaceHeight = calculateSurfaceHeight(baseChunkWorldPosition.x, i);

			// Check to see if the surface height is within the bounds of the chunk.
			// Since there may be some surface chunks above or below the actual surface, this check is necessary
			// so trees don't grow in the ground or in the air
			if (surfaceHeight < baseChunkWorldPosition.y || surfaceHeight >= baseChunkWorldPosition.y + CHUNK_SIZE * BLOCK_SIZE)
				continue;

			int baseBlockX = i;
			int baseBlockY = (surfaceHeight / BLOCK_SIZE) % CHUNK_SIZE;
			while (baseBlockY < 0) // Adjust negative block y positions so that they are positive
				baseBlockY += CHUNK_SIZE;

			// Get the height of the tree
			float treeHeightNoise = SimplexNoise::noise((baseChunkWorldPosition.x + i * BLOCK_SIZE + 1) / TREE_SMOOTHNESS);
			int treeHeight = (int)roundf(map(treeHeightNoise, -1, 1, TREE_BASE_HEIGHT_MIN, TREE_BASE_HEIGHT_MAX));

			// Keep a set of block pointers that make up the tree for leaf generation later
			std::unordered_set<const Block*> treeBlocks;

			Chunk* currentChunk = baseChunk;
			int currentTreeHeight = baseBlockY;
			float treeBranchHeightNoise = 0;
			for (int j = 1; j <= treeHeight; j++)
			{
				currentTreeHeight = baseBlockY + j;
				if (currentTreeHeight >= CHUNK_SIZE)
				{
					currentTreeHeight -= CHUNK_SIZE;
					baseBlockY = -j - 1; // Adjust the base block y value so that on the next iteration
										// the current tree height will be 0, since it will be in a new chunk

					// The tree has outstretched the chunk, either get or generate the next one
					glm::vec2 newChunkPosition = currentChunk->chunkPosition;
					newChunkPosition.y++;

					auto chunkIt = m_chunks.find(newChunkPosition);
					if (chunkIt == m_chunks.end())
					{
						// Chunk needs to be generated
						currentChunk = createChunk(newChunkPosition);
						queueGenChunk(currentChunk);

						std::unique_lock<std::mutex> lock(currentChunk->mutex);
						currentChunk->cv.wait(lock); // Waits for the chunk to be fully generated
					}
					else
					{
						// Chunk already exists
						currentChunk = chunkIt->second;

						std::unique_lock<std::mutex> lock(currentChunk->mutex);
						if (!currentChunk->hasGenerated) // Chunk exists, but hasn't fully generated yet
						{
							currentChunk->cv.wait(lock); // Waits for the chunk to be fully generated
						}
					}

					// Only add the chunk to the modified chunks list if it hasn't been added already, since there can be
					// multiple trees in the same chunk
					if (std::find(modifiedChunks.begin(), modifiedChunks.end(), currentChunk) == modifiedChunks.end())
						modifiedChunks.push_back(currentChunk);
				}

				treeBranchHeightNoise = SimplexNoise::noise((baseChunkWorldPosition.x + i * BLOCK_SIZE + 1) / TREE_SMOOTHNESS,
					(baseChunkWorldPosition.y + currentTreeHeight * BLOCK_SIZE + 1) / TREE_SMOOTHNESS);
				int treeBranchHeight = (int)roundf(map(treeBranchHeightNoise, -1, 1, TREE_BRANCH_HEIGHT_MIN, TREE_BRANCH_HEIGHT_MAX));
				if (j > treeBranchHeight)
				{
					// Try to generate branches on the left and right
					genTreeBranches(currentChunk, baseBlockX, currentTreeHeight, (float)currentTreeHeight / treeHeight, -1, treeBlocks);
					genTreeBranches(currentChunk, baseBlockX, currentTreeHeight, (float)currentTreeHeight / treeHeight, 1, treeBlocks);
				}

				// Take ownership of the chunk while building the tree
				std::unique_lock<std::mutex> lock(currentChunk->mutex);

				Block& block = currentChunk->blocks[baseBlockX + CHUNK_SIZE * currentTreeHeight];
				if (block.blockType != AIR) break;

				currentChunk->blockCount[block.blockType]--;
				setBlock(block, WOOD, block.sprite.transform.position, 0);
				currentChunk->blockCount[WOOD]++;

				treeBlocks.insert(&block);
			}

			// Use the noise from the block at the top of the tree to determine the leaf radius
			int leafRadius = (int)roundf(map(treeBranchHeightNoise - (1 - currentTreeHeight / CHUNK_SIZE), -1, 1, TREE_TOP_LEAF_RADIUS_MIN, TREE_TOP_LEAF_RADIUS_MAX));
			glm::vec2 topTreeIndices = glm::vec2(baseBlockX, currentTreeHeight);
			for (ptrdiff_t k = -leafRadius; k <= leafRadius; k++)
			{
				for (ptrdiff_t j = -leafRadius; j <= leafRadius; j++)
				{
					// Don't put a leaf block at the bottom middle, because a flat leaf bottom looks better
					if (k == -leafRadius && j == 0) continue;

					// Checks if the current leaf indices are within the chunk
					glm::vec2 currentLeafIndices = glm::vec2(baseBlockX + j, currentTreeHeight + k);
					if (currentLeafIndices.x < 0 || currentLeafIndices.x >= CHUNK_SIZE ||
						currentLeafIndices.y < 0 || currentLeafIndices.y >= CHUNK_SIZE) continue;

					// Distance check
					if ((currentLeafIndices.x - topTreeIndices.x) * (currentLeafIndices.x - topTreeIndices.x) +
						(currentLeafIndices.y - topTreeIndices.y) * (currentLeafIndices.y - topTreeIndices.y) <= leafRadius * leafRadius)
					{
						// Take ownership of the chunk while building the leaf
						std::unique_lock<std::mutex> lock(currentChunk->mutex);

						Block& leafBlock = currentChunk->blocks[(size_t)currentLeafIndices.x + CHUNK_SIZE * (size_t)currentLeafIndices.y];
						if ((leafBlock.blockType == WOOD || leafBlock.blockType == BRANCH) && treeBlocks.count(&leafBlock) == 0) continue;
						if (leafBlock.blockType != AIR && leafBlock.blockType != WOOD && leafBlock.blockType != BRANCH) continue;
						
						currentChunk->blockCount[leafBlock.blockType]--;
						setBlock(leafBlock, LEAF, leafBlock.sprite.transform.position, 0);
						currentChunk->blockCount[LEAF]++;
					}
				}
			}
		}
	}

	return modifiedChunks;
}

void Terrain::genTreeBranches(Chunk* chunk, int blockX, int treeHeight, float heightRatio, int direction, std::unordered_set<const Block*>& treeBlocks)
{
	glm::vec2 baseChunkWorldPosition = chunkToWorldCoords(chunk->chunkPosition);
	
	// Generate noise to see if we should have a branch at this position
	float branchNoise = SimplexNoise::noise(baseChunkWorldPosition.x + blockX * BLOCK_SIZE, baseChunkWorldPosition.y + treeHeight * BLOCK_SIZE);

	if (direction < 0)
	{
		if (branchNoise < 0.5f) return;
	}
	else
	{
		if (branchNoise > -0.5f) return;
	}
	

	// Calculate how long the branch is in blocks
	float branchLengthNoise = SimplexNoise::noise((baseChunkWorldPosition.x + (blockX * BLOCK_SIZE) + (direction * BLOCK_SIZE)) / TREE_SMOOTHNESS,
		(baseChunkWorldPosition.y + treeHeight * BLOCK_SIZE) / TREE_SMOOTHNESS);
	int branchLength = (int)roundf(map(branchLengthNoise, -1, 1, TREE_BRANCH_LENGTH_MIN, TREE_BRANCH_LENGTH_MAX));
	
	int currentBranchHeight = treeHeight;
	glm::vec2 blockIndices;
	for (size_t i = 1; i <= branchLength; i++)
	{
		// Calculate the block indices for the block of the branch

		// Check if the branch should curve up
		float branchVarianceNoise = SimplexNoise::noise((baseChunkWorldPosition.x + (blockX * BLOCK_SIZE) + (direction * i * BLOCK_SIZE)),
			(baseChunkWorldPosition.y + currentBranchHeight * BLOCK_SIZE));

		if (branchVarianceNoise > (1 - heightRatio) + (1 - ((float)i / branchLength)))
			currentBranchHeight++;
		
		blockIndices = glm::vec2(blockX + direction * (int)i, currentBranchHeight);

		// Check if that block is outside the chunk
		if (blockIndices.x < 0 || blockIndices.x >= CHUNK_SIZE || 
			blockIndices.y < 0 || blockIndices.y >= CHUNK_SIZE)
		{
			continue;
		}

		glm::vec2 topLeafIndices = glm::vec2(blockIndices.x, blockIndices.y + 1);
		glm::vec2 bottomLeafIndices = glm::vec2(blockIndices.x, blockIndices.y - 1);

		// Take ownership of the chunk while building the branch
		std::unique_lock<std::mutex> lock(chunk->mutex);

		Block& block = chunk->blocks[(size_t)blockIndices.x + CHUNK_SIZE * (size_t)blockIndices.y];
		if (block.blockType != AIR) return;

		chunk->blockCount[block.blockType]--;
		setBlock(block, BRANCH, block.sprite.transform.position, 0);
		chunk->blockCount[BRANCH]++;

		treeBlocks.insert(&block);

		/*if (topLeafIndices.y < CHUNK_SIZE)
		{
			Block& topLeafBlock = chunk->blocks[(size_t)topLeafIndices.x + CHUNK_SIZE * (size_t)topLeafIndices.y];
			if (topLeafBlock.blockType != AIR) return;

			chunk->blockCount[topLeafBlock.blockType]--;
			setBlock(topLeafBlock, LEAF, topLeafBlock.sprite.transform.position, 0);
			chunk->blockCount[LEAF]++;
		}

		if (bottomLeafIndices.y >= 0)
		{
			Block& bottomLeafBlock = chunk->blocks[(size_t)bottomLeafIndices.x + CHUNK_SIZE * (size_t)bottomLeafIndices.y];
			if (bottomLeafBlock.blockType != AIR) return;

			chunk->blockCount[bottomLeafBlock.blockType]--;
			setBlock(bottomLeafBlock, LEAF, bottomLeafBlock.sprite.transform.position, 0);
			chunk->blockCount[LEAF]++;
		}*/
	}

	/*if (branchLength > 0)
	{
		glm::vec2 endLeafIndices = glm::vec2(blockIndices.x + direction, blockIndices.y);
		if (endLeafIndices.x >= 0 && endLeafIndices.x < CHUNK_SIZE &&
			endLeafIndices.y >= 0 && endLeafIndices.y < CHUNK_SIZE)
		{
			Block& endLeafBlock = chunk->blocks[(size_t)endLeafIndices.x + CHUNK_SIZE * (size_t)endLeafIndices.y];
			if (endLeafBlock.blockType != AIR) return;

			chunk->blockCount[endLeafBlock.blockType]--;
			setBlock(endLeafBlock, LEAF, endLeafBlock.sprite.transform.position, 0);
			chunk->blockCount[LEAF]++;
		}
	}*/
}

void Terrain::sortBlockIndexMap(const Block blocks[CHUNK_SIZE * CHUNK_SIZE], unsigned int blockIndexMap[CHUNK_SIZE * CHUNK_SIZE])
{
	std::sort(blockIndexMap, blockIndexMap + CHUNK_SIZE * CHUNK_SIZE, [&blocks](const unsigned int& index1, const unsigned int &index2)
	{
		return blocks[index1].blockType < blocks[index2].blockType;
	});
}

void Terrain::updateDrawingBuffers(const size_t containerIndex)
{
	const ChunkContainer& chunkContainer = m_chunkContainers[containerIndex];

	// Cache the blocks pointer and the blockIndexMap pointer
	Block* blocks = chunkContainer.chunk->blocks;
	unsigned int* blockIndexMap = chunkContainer.chunk->blockIndexMap;

	// Collect the container's world positions (in sorted order)
	glm::vec2 worldPositions[CHUNK_SIZE * CHUNK_SIZE];
	for (size_t i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		worldPositions[i] = blocks[blockIndexMap[i]].sprite.transform.position;
	}

	// Collect the container's uv offset indices (in sorted order)
	unsigned int uvOffsetIndices[CHUNK_SIZE * CHUNK_SIZE];
	for (size_t i = 0; i < CHUNK_SIZE * CHUNK_SIZE; i++)
	{
		uvOffsetIndices[i] = blocks[blockIndexMap[i]].sprite.renderData.uvOffsetIndex;
	}

	// Update the container's world positions
	glBindBuffer(GL_ARRAY_BUFFER, chunkContainer.worldPositionsVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2) * CHUNK_SIZE * CHUNK_SIZE, &worldPositions[0][0]);

	// Update the container's uv offset indices
	glBindBuffer(GL_ARRAY_BUFFER, chunkContainer.uvOffsetIndicesVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(unsigned int) * CHUNK_SIZE * CHUNK_SIZE, &uvOffsetIndices[0]);
}

void Terrain::clearWorldPositions(const ChunkContainer& chunkContainer)
{
	glm::vec2 worldPositions[CHUNK_SIZE * CHUNK_SIZE];
	memset(worldPositions, 0, CHUNK_SIZE * CHUNK_SIZE);

	glBindBuffer(GL_ARRAY_BUFFER, chunkContainer.worldPositionsVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec2) * CHUNK_SIZE * CHUNK_SIZE, &worldPositions[0][0]);
}

void Terrain::clearUVOffsetIndices(const ChunkContainer& chunkContainer)
{
	unsigned int uvOffsetIndices[CHUNK_SIZE * CHUNK_SIZE];
	memset(uvOffsetIndices, 0, CHUNK_SIZE * CHUNK_SIZE);

	glBindBuffer(GL_ARRAY_BUFFER, chunkContainer.uvOffsetIndicesVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(unsigned int) * CHUNK_SIZE * CHUNK_SIZE, &uvOffsetIndices[0]);
}

void Terrain::checkThreadsFinished()
{
	// Remove any finished chunk gen threads
	for (size_t i = 0; i < m_genChunkThreads.size(); i++)
	{
		if (m_genChunkThreads[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			Chunk* chunk = m_genChunkThreads[i].get();

			// Launch a post gen thread so that the chunk can add any additional post gen features
			m_postGenThreads.push_back(std::async(std::launch::async, &Terrain::postGenChunkThreaded, this, chunk));

			// The thread is done, remove it from the list
			m_genChunkThreads.erase(m_genChunkThreads.begin() + i);
			i--;
		}
	}

	// Remove any finished post generation threads
	for (size_t i = 0; i < m_postGenThreads.size(); i++)
	{
		if (m_postGenThreads[i].wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			// The chunks have fully loaded
			std::vector<Chunk*> modifiedChunks = m_postGenThreads[i].get();
			for (size_t i = 0; i < modifiedChunks.size(); i++)
			{
				modifiedChunks[i]->hasFullyLoaded = true;

				// Resort the block index map since the chunks was modified
				sortBlockIndexMap(modifiedChunks[i]->blocks, modifiedChunks[i]->blockIndexMap);
				
				// Update the drawing buffers with the newly sorted block indices
				if (modifiedChunks[i]->containerIndex > -1)
					updateDrawingBuffers(modifiedChunks[i]->containerIndex);
			}

			// The thread is done, remove it from the list
			m_postGenThreads.erase(m_postGenThreads.begin() + i);
			i--;
		}
	}
}

void Terrain::checkShiftChunkContainers()
{
	glm::vec2 cameraPosition = m_camera.getPosition();
	int cameraWidth = m_camera.getWidth();
	int cameraHeight = m_camera.getHeight();

	// List of chunks that need to be generated
	std::vector<Chunk*> chunks;

	// Check if we need to update the chunk containers from the right
	if (cameraPosition.x + cameraWidth * 0.5f + CAMERA_VIEW_BUFFER_CONTAINER_REASSIGN >= m_chunkContainerOriginWorldPos.x + CHUNK_SIZE * BLOCK_SIZE * (CHUNK_CONTAINER_DISTANCE + 1))
	{
		// Converts the chunk containers' origin to chunk coordinates
		glm::vec2 chunkContainerOriginPos = worldToChunkCoords(m_chunkContainerOriginWorldPos);

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

				updateDrawingBuffers(chunkContainerIndex - 1);
				chunkContainerIndex++;
			}
		}

		// Assign the rightmost containers, or generate them if necessary
		for (int i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			Chunk* chunk = nullptr;
			glm::vec2 chunkPosition = glm::vec2(chunkContainerOriginPos.x + (CHUNK_CONTAINER_DISTANCE + 1), chunkContainerOriginPos.y + i);
			if (m_chunks.find(chunkPosition) == m_chunks.end())
			{
				chunk = createChunk(chunkPosition);
				chunks.push_back(chunk);
			}
			else
				chunk = m_chunks[chunkPosition];
				

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
		glm::vec2 chunkContainerOriginPos = worldToChunkCoords(m_chunkContainerOriginWorldPos);

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

				updateDrawingBuffers(chunkContainerIndex + 1);
				chunkContainerIndex--;
			}
		}

		// Assign the leftmost containers, or generate them if necessary
		for (int i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			Chunk* chunk = nullptr;
			glm::vec2 chunkPosition = glm::vec2(chunkContainerOriginPos.x - 1, chunkContainerOriginPos.y + i);
			if (m_chunks.find(chunkPosition) == m_chunks.end())
			{
				chunk = createChunk(chunkPosition);
				chunks.push_back(chunk);
			}
			else
				chunk = m_chunks[chunkPosition];

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
		glm::vec2 chunkContainerOriginPos = worldToChunkCoords(m_chunkContainerOriginWorldPos);

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

				updateDrawingBuffers(chunkContainerIndex - (CHUNK_CONTAINER_DISTANCE + 1));
				chunkContainerIndex++;
			}
		}

		// Assign the topmost containers, or generate them if necessary
		for (int i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			Chunk* chunk = nullptr;
			glm::vec2 chunkPosition = glm::vec2(chunkContainerOriginPos.x + i, chunkContainerOriginPos.y + (CHUNK_CONTAINER_DISTANCE + 1));
			if (m_chunks.find(chunkPosition) == m_chunks.end())
			{
				chunk = createChunk(chunkPosition);
				chunks.push_back(chunk);
			}
			else
				chunk = m_chunks[chunkPosition];

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
		glm::vec2 chunkContainerOriginPos = worldToChunkCoords(m_chunkContainerOriginWorldPos);

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

				updateDrawingBuffers(chunkContainerIndex + (CHUNK_CONTAINER_DISTANCE + 1));
				chunkContainerIndex--;
			}
		}

		// Assign the bottommost containers, or generate them if necessary
		for (int i = 0; i < CHUNK_CONTAINER_DISTANCE + 1; i++)
		{
			Chunk* chunk = nullptr;
			glm::vec2 chunkPosition = glm::vec2(chunkContainerOriginPos.x + i, chunkContainerOriginPos.y - 1);
			if (m_chunks.find(chunkPosition) == m_chunks.end())
			{
				chunk = createChunk(chunkPosition);
				chunks.push_back(chunk);
			}
			else
				chunk = m_chunks[chunkPosition];

			chunk->containerIndex = i;

			ChunkContainer& chunkContainer = m_chunkContainers[chunk->containerIndex];
			chunkContainer.chunk = chunk;

			if (chunk->hasFullyLoaded)
				updateDrawingBuffers(chunk->containerIndex);
		}

		// Shift the chunk container origin by 1 chunk
		m_chunkContainerOriginWorldPos.y -= CHUNK_SIZE * BLOCK_SIZE;
	}

	// Queue the chunks for generation
	queueGenChunks(chunks);
}

void Terrain::checkGenChunks()
{
	std::vector<Chunk*> chunks;

	glm::vec2 cameraPosition = m_camera.getPosition();
	int cameraWidth = m_camera.getWidth();
	int cameraHeight = m_camera.getHeight();

	glm::vec2 cameraChunkPosition = worldToChunkCoords(cameraPosition);

	// Check for chunks in a square around the camera
	for (ptrdiff_t j = (ptrdiff_t)cameraChunkPosition.y - CAMERA_VIEW_BUFFER_GEN; j <= cameraChunkPosition.y + CAMERA_VIEW_BUFFER_GEN; j++)
	{
		for (ptrdiff_t i = (ptrdiff_t)cameraChunkPosition.x - CAMERA_VIEW_BUFFER_GEN; i <= cameraChunkPosition.x + CAMERA_VIEW_BUFFER_GEN; i++)
		{
			glm::vec2 chunkPosition = glm::vec2(i, j);
			if (m_chunks.count(chunkPosition) == 0)
			{
				Chunk* chunk = createChunk(chunkPosition);
				chunks.push_back(chunk);
			}
		}
	}

	if (!chunks.empty())
		queueGenChunks(chunks);
}

void Terrain::checkUnloadChunks()
{
	glm::vec2 cameraPosition = m_camera.getPosition();
	int cameraWidth = m_camera.getWidth();
	int cameraHeight = m_camera.getHeight();

	std::vector<glm::vec2> chunksToUnload;
	for (auto it = m_chunks.begin(); it != m_chunks.end(); it++)
	{
		// Don't unload chunks that haven't been fully loaded yet, as that can cause multithreaded crashes
		if (!it->second->hasFullyLoaded) continue;

		glm::vec2 chunkWorldPosition = chunkToWorldCoords(it->first);
		float worldUnloadBuffer = CAMERA_VIEW_BUFFER_UNLOAD * CHUNK_SIZE * BLOCK_SIZE;
		if (chunkWorldPosition.x + CHUNK_SIZE * BLOCK_SIZE < cameraPosition.x - cameraWidth * 0.5f - worldUnloadBuffer ||
			chunkWorldPosition.x > cameraPosition.x + cameraWidth * 0.5f + worldUnloadBuffer ||
			chunkWorldPosition.y + CHUNK_SIZE * BLOCK_SIZE < cameraPosition.y - cameraHeight * 0.5f - worldUnloadBuffer ||
			chunkWorldPosition.y > cameraPosition.y + cameraHeight * 0.5f + worldUnloadBuffer)
		{
			delete it->second;
			it->second = nullptr;
			chunksToUnload.push_back(it->first);
		}
	}

	for (size_t i = 0; i < chunksToUnload.size(); i++)
	{
		m_chunks.erase(chunksToUnload[i]);
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

int Terrain::calculateSurfaceHeight(float chunkWorldPositionX, size_t blockX)
{
	return (int)(roundf(m_terrainNoise->fractal(SURFACE_OCTAVES, (chunkWorldPositionX + blockX * BLOCK_SIZE + 1) / SMOOTHNESS) * HEIGHT_FLUX / BLOCK_SIZE) * BLOCK_SIZE);
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
	return glm::vec2((int)(worldPosition.x / BLOCK_SIZE) * BLOCK_SIZE, (int)(worldPosition.y / BLOCK_SIZE) * BLOCK_SIZE);
}
