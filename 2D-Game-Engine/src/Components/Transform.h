#pragma once
#include "Component.h"

struct Transform : public Component
{
	Transform(size_t entityID) : Component(entityID) {}

	glm::vec2 position;
	glm::vec2 size;
};