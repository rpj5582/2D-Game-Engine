#include "stdafx.h"
#include "PlayerController.h"

#include "Input.h"

PlayerController::PlayerController(size_t playerID) : m_playerID(playerID), m_grounded(false), m_physicsSystem(*PhysicsSystem::getInstance())
{
}

size_t PlayerController::getPlayerID() const
{
	return m_playerID;
}

void PlayerController::update(float deltaTime)
{
	if (Input::getInstance()->isKeyHeld(GLFW_KEY_LEFT))
	{
		m_physicsSystem.applyImpulse(m_playerID, glm::vec2(-100, 0));
	}

	if (Input::getInstance()->isKeyHeld(GLFW_KEY_RIGHT))
	{
		m_physicsSystem.applyImpulse(m_playerID, glm::vec2(100, 0));
	}

	if (m_grounded && Input::getInstance()->isKeyPressed(GLFW_KEY_SPACE))
	{
		m_physicsSystem.applyImpulse(m_playerID, glm::vec2(0, 2500));
	}
}

void PlayerController::beginContact(const PhysicsObject* physicsObject, const PhysicsObject* other)
{
	m_grounded = true;
}

void PlayerController::endContact(const PhysicsObject* physicsObject, const PhysicsObject* other)
{
	m_grounded = false;
}
