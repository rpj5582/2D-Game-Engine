#pragma once

#include "Sprite.h"

#define BUCKET_SIZE 64

struct CollisionPair
{
	Sprite& sprite1;
	Sprite& sprite2;

	bool operator==(const CollisionPair& other) const;
};

namespace std
{
	template<>
	struct hash<CollisionPair>
	{
		size_t operator()(const CollisionPair& collisionPair) const
		{
			return hash<size_t>()(collisionPair.sprite1.id) ^ hash<size_t>()(collisionPair.sprite2.id);
		}
	};
}

class CollisionHandler
{
public:
	CollisionHandler();
	~CollisionHandler();

	std::vector<CollisionPair>& getCollisionPairs();
	void insertIntoGrid(Sprite* sprites[], size_t size);
	void clearGrid();

	bool calculateResolution(const CollisionPair& collisionPair, glm::vec2* mtv);
	bool calculateResolution(const CollisionPair& collisionPair, float cornerCutout, glm::vec2* mtv);

private:
	bool testAxis(float t1Min, float t1Max, float t2Min, float t2Max, glm::vec2 axis, float& minOverlapSquared, glm::vec2& smallestAxis);
	inline bool isOverlapping(float t1Min, float t1Max, float t2Min, float t2Max, float* overlap) const;

	std::unordered_map<glm::vec2, std::vector<Sprite*>> m_grid;
	std::vector<CollisionPair> m_collisionPairs;
};