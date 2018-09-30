#include "stdafx.h"
#include "CollisionHandler.h"

CollisionHandler::CollisionHandler()
{
}

CollisionHandler::~CollisionHandler()
{
}

bool CollisionHandler::calculateResolution(const CollisionPair& collisionPair, glm::vec2* mtv)
{
	float minOverlapSquared = FLT_MAX;
	glm::vec2 smallestAxis = glm::vec2();
	glm::vec2 axes[2] = { glm::vec2(1, 0), glm::vec2(0, 1) };

	for (unsigned int i = 0; i < 2; i++)
	{
		float t1Min = collisionPair.sprite1.transform.position[i] - collisionPair.sprite1.transform.size[i] * 0.5f;
		float t1Max = collisionPair.sprite1.transform.position[i] + collisionPair.sprite1.transform.size[i] * 0.5f;
		float t2Min = collisionPair.sprite2.transform.position[i] - collisionPair.sprite2.transform.size[i] * 0.5f;
		float t2Max = collisionPair.sprite2.transform.position[i] + collisionPair.sprite2.transform.size[i] * 0.5f;

		if (!testAxis(t1Min, t1Max, t2Min, t2Max, axes[i], minOverlapSquared, smallestAxis)) return false;
	}

	*mtv = smallestAxis;
	return true;
}

bool CollisionHandler::calculateResolution(const CollisionPair& collisionPair, float cornerCutout, glm::vec2* mtv)
{
	glm::vec2 mtvX = glm::vec2();
	glm::vec2 mtvY = glm::vec2();

	float minOverlapSquared = FLT_MAX;
	glm::vec2 smallestAxis = glm::vec2();
	glm::vec2 axes[2] = { glm::vec2(1, 0), glm::vec2(0, 1) };

	bool isColliding = true;
	for (unsigned int i = 0; i < 2; i++)
	{
		float t1Min = collisionPair.sprite1.transform.position[i] - collisionPair.sprite1.transform.size[i] * 0.5f + cornerCutout * i;
		float t1Max = collisionPair.sprite1.transform.position[i] + collisionPair.sprite1.transform.size[i] * 0.5f - cornerCutout * i;
		float t2Min = collisionPair.sprite2.transform.position[i] - collisionPair.sprite2.transform.size[i] * 0.5f + cornerCutout * i;
		float t2Max = collisionPair.sprite2.transform.position[i] + collisionPair.sprite2.transform.size[i] * 0.5f - cornerCutout * i;

		if (!testAxis(t1Min, t1Max, t2Min, t2Max, axes[i], minOverlapSquared, smallestAxis))
		{
			isColliding = false;
			break;
		}
	}

	if (isColliding)
		mtvX = smallestAxis;

	minOverlapSquared = FLT_MAX;
	smallestAxis = glm::vec2();

	isColliding = true;
	for (unsigned int i = 0; i < 2; i++)
	{
		float t1Min = collisionPair.sprite1.transform.position[i] - collisionPair.sprite1.transform.size[i] * 0.5f + cornerCutout * ((i + 1) % 2);
		float t1Max = collisionPair.sprite1.transform.position[i] + collisionPair.sprite1.transform.size[i] * 0.5f - cornerCutout * ((i + 1) % 2);
		float t2Min = collisionPair.sprite2.transform.position[i] - collisionPair.sprite2.transform.size[i] * 0.5f + cornerCutout * ((i + 1) % 2);
		float t2Max = collisionPair.sprite2.transform.position[i] + collisionPair.sprite2.transform.size[i] * 0.5f - cornerCutout * ((i + 1) % 2);

		if (!testAxis(t1Min, t1Max, t2Min, t2Max, axes[i], minOverlapSquared, smallestAxis))
		{
			isColliding = false;
			break;
		}
	}

	if (isColliding)
		mtvY = smallestAxis;

	*mtv = glm::vec2(mtvX.x, mtvY.y);
	return true;
}

bool CollisionHandler::testAxis(float t1Min, float t1Max, float t2Min, float t2Max, glm::vec2 axis, float& minOverlapSquared, glm::vec2& smallestAxis)
{
	float overlap;
	if (!isOverlapping(t1Min, t1Max, t2Min, t2Max, &overlap)) return false;

	glm::vec2 separatingAxis = axis * overlap;
	float separatingAxisLengthSquared = glm::dot(separatingAxis, separatingAxis);

	if (separatingAxisLengthSquared < minOverlapSquared)
	{
		minOverlapSquared = separatingAxisLengthSquared;
		smallestAxis = separatingAxis;
	}

	return true;
}

inline bool CollisionHandler::isOverlapping(float t1Min, float t1Max, float t2Min, float t2Max, float* overlap) const
{
	float leftOverlap = t2Max - t1Min; // Overlapping left of t1
	float rightOverlap = t1Max - t2Min; // Overlapping right of t1

	// Not overlapping, return
	if (leftOverlap <= 0 || rightOverlap <= 0) return false;

	*overlap = leftOverlap < rightOverlap ? leftOverlap : -rightOverlap;
	return true;
}

std::vector<CollisionPair>& CollisionHandler::getCollisionPairs()
{
	m_collisionPairs.clear();

	for (auto it = m_grid.begin(); it != m_grid.end(); it++)
	{
		const std::vector<Sprite*>& sprites = it->second;
		for (size_t j = 0; j < sprites.size(); j++)
		{
			for (size_t i = j; i < sprites.size(); i++)
			{
				if (sprites[i] != sprites[j])
				{
					m_collisionPairs.push_back({ *sprites[j], *sprites[i] });
				}
			}
		}
	}

	return m_collisionPairs;
}

void CollisionHandler::insertIntoGrid(Sprite* sprites[], size_t size)
{
	for (size_t k = 0; k < size; k++)
	{
		glm::vec2 position = sprites[k]->transform.position;
		
		glm::vec2 minPosition = position - sprites[k]->transform.size / 2.0f;
		glm::vec2 maxPosition = position + sprites[k]->transform.size / 2.0f;

		glm::vec2 minCellPosition = glm::vec2((int)(minPosition.x / BUCKET_SIZE), (int)(minPosition.y / BUCKET_SIZE));
		glm::vec2 maxCellPosition = glm::vec2((int)(maxPosition.x / BUCKET_SIZE), (int)(maxPosition.y / BUCKET_SIZE));
		
		for (int j = (int)minCellPosition.y; j <= maxCellPosition.y; j++)
		{
			for (int i = (int)minCellPosition.x; i <= maxCellPosition.x; i++)
			{
				m_grid[glm::vec2(i, j)].push_back(sprites[k]);
			}
		}
	}
}

void CollisionHandler::clearGrid()
{
	m_grid.clear();
}

bool CollisionPair::operator==(const CollisionPair& other) const
{
	return sprite1.id == other.sprite1.id && sprite2.id == other.sprite2.id || 
		sprite1.id == other.sprite2.id && sprite2.id == other.sprite1.id;
}
