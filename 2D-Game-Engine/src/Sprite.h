#pragma once

#include <glm.hpp>

struct Transform
{
	glm::vec2 position;
	glm::vec2 size;
	float rotation;
};

struct RenderData
{
	unsigned int textureID;
	unsigned int shaderID;
};

struct Sprite
{
	Transform transform;
	RenderData renderData;
};