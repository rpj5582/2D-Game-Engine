#include "stdafx.h"
#include "PlayerController.h"

#include "AssetManager.h"
#include "Input.h"

PlayerController::PlayerController(const Transform& startingTransform)
{
	AssetManager* assetManager = AssetManager::getInstance();
	unsigned int playerTextureID = assetManager->loadTexture("player", "textures/player.png");
	unsigned int defaultShaderID = assetManager->getShader("defaultShader");

	m_player = Sprite(
		startingTransform,
		RenderData(Texture(glm::vec2(32, 64), playerTextureID), defaultShaderID, glm::vec2(32, 64), 0, nullptr)
	);
}

PlayerController::~PlayerController()
{
}

void PlayerController::update(float deltaTime)
{
	//if (Input::getInstance()->isKeyHeld(GLFW_KEY_LEFT))
	//{
	//	m_velocity.x -= 200;
	//}

	//if (Input::getInstance()->isKeyHeld(GLFW_KEY_RIGHT))
	//{
	//	m_velocity.x += 200;
	//}

	//if (Input::getInstance()->isKeyPressed(GLFW_KEY_SPACE))
	//{
	//	m_velocity.y += 2000;
	//}

	//// Gravity
	//m_velocity.y -= 200;

	//translate(m_velocity * deltaTime);
	//m_velocity = glm::vec2();

	if (Input::getInstance()->isKeyHeld(GLFW_KEY_LEFT))
	{
		m_player.transform.position.x -= 1000 * deltaTime;
	}

	if (Input::getInstance()->isKeyHeld(GLFW_KEY_RIGHT))
	{
		m_player.transform.position.x += 1000 * deltaTime;
	}

	if (Input::getInstance()->isKeyHeld(GLFW_KEY_UP))
	{
		m_player.transform.position.y += 1000 * deltaTime;
	}

	if (Input::getInstance()->isKeyHeld(GLFW_KEY_DOWN))
	{
		m_player.transform.position.y -= 1000 * deltaTime;
	}
}

void PlayerController::translate(glm::vec2 delta)
{
	m_player.transform.position += delta;
}

void PlayerController::resize(glm::vec2 delta)
{
	m_player.transform.size += delta;
}

glm::vec2 PlayerController::getPosition() const
{
	return m_player.transform.position;
}

glm::vec2 PlayerController::getSize() const
{
	return m_player.transform.size;
}

Sprite* PlayerController::getSprite()
{
	return &m_player;
}
