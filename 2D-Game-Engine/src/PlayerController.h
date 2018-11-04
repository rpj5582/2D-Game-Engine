#pragma once

#include "Systems/PhysicsSystem.h"

class PlayerController
{
public:
	PlayerController(size_t playerID);

	size_t getPlayerID() const;

	void update(float deltaTime);

	void beginContact(const PhysicsObject* physicsObject, const PhysicsObject* other);
	void endContact(const PhysicsObject* physicsObject, const PhysicsObject* other);

private:
	PhysicsSystem& m_physicsSystem;

	size_t m_playerID;
	bool m_grounded;
};