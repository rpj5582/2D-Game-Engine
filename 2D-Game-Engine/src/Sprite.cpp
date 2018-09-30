#include "stdafx.h"
#include "Sprite.h"

size_t Sprite::m_uniqueIDCounter = 0;

Transform::Transform(glm::vec2 position, glm::vec2 size)
	: position(position), size(size) {}

void Transform::translate(glm::vec2 delta)
{
	position += delta;
}

void Transform::resize(glm::vec2 delta)
{
	size += delta;
}

Texture::Texture(glm::vec2 dimensions, unsigned int id) : dimensions(dimensions), id(id) {}

RenderData::RenderData(Texture texture, unsigned int shaderID, glm::vec2 tileDimensions, unsigned int uvOffsetIndex, glm::vec2 uvOffsets[MAX_ANIMATION_LENGTH])
	: texture(texture), shaderID(shaderID), tileDimensions(tileDimensions), uvOffsetIndex(uvOffsetIndex)
{
	if (uvOffsets)
		memcpy_s(this->uvOffsets, sizeof(glm::vec2) * MAX_ANIMATION_LENGTH, uvOffsets, sizeof(glm::vec2) * MAX_ANIMATION_LENGTH);
}

Sprite::Sprite(const Transform& transform, const RenderData& renderData) : transform(transform), renderData(renderData), id(++m_uniqueIDCounter) {}
