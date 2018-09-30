#pragma once

#include "Sprite.h"

class PlayerController
{
public:
	PlayerController(const Transform& startingTransform);
	~PlayerController();

	void update(float deltaTime);

	void translate(glm::vec2 delta);
	void resize(glm::vec2 delta);

	glm::vec2 getPosition() const;
	glm::vec2 getSize() const;

	Sprite* getSprite();

private:
	Sprite m_player;
	glm::vec2 m_velocity;
};