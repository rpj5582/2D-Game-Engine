#pragma once

#include <glm.hpp>

#define MAX_ANIMATION_LENGTH 30

struct Transform
{
	glm::vec2 position;
	glm::vec2 size;

	Transform() = default;
	Transform(glm::vec2 position, glm::vec2 size);

	void translate(glm::vec2 delta);
	void resize(glm::vec2 delta);
};

struct Texture
{
	glm::vec2 dimensions;
	unsigned int id;

	Texture() = default;
	Texture(glm::vec2 dimensions, unsigned int id);
};

struct RenderData
{
	glm::vec2 tileDimensions;
	glm::vec2 uvOffsets[MAX_ANIMATION_LENGTH];
	
	Texture texture;
	
	unsigned int shaderID;
	unsigned int uvOffsetIndex;

	RenderData() = default;
	RenderData(Texture texture, unsigned int shaderID, glm::vec2 tileDimensions, unsigned int uvOffsetIndex, glm::vec2 uvOffsets[MAX_ANIMATION_LENGTH]);
};

struct Sprite
{
	RenderData renderData;
	Transform transform;
	size_t id;

	Sprite() = default;
	Sprite(const Transform& transform, const RenderData& renderData);

private:
	static size_t m_uniqueIDCounter;
};