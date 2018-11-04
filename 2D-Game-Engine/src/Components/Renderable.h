#pragma once
#include "Component.h"

#define MAX_ANIMATION_LENGTH 30

struct Texture
{
	Texture() : dimensions(glm::vec2()), id(0) {}
	Texture(glm::vec2 dimensions, unsigned int id) : dimensions(dimensions), id(id) {}

	glm::vec2 dimensions;
	unsigned int id;
};

struct Renderable : public Component
{
	Renderable(size_t entityID) : Component(entityID) {}

	void operator=(const Renderable& other)
	{
		texture = other.texture;
		tileDimensions = other.tileDimensions;
		uvOffsetIndex = other.uvOffsetIndex;
		shaderID = other.shaderID;

		if (other.uvOffsets)
			memcpy_s(uvOffsets, sizeof(glm::vec2) * MAX_ANIMATION_LENGTH, other.uvOffsets, sizeof(glm::vec2) * MAX_ANIMATION_LENGTH);
		else
			memset(uvOffsets, 0, sizeof(glm::vec2) * MAX_ANIMATION_LENGTH);
	}

	Texture texture;
	glm::vec2 tileDimensions;
	glm::vec2 uvOffsets[MAX_ANIMATION_LENGTH];
	unsigned int uvOffsetIndex;
	unsigned int shaderID;
};