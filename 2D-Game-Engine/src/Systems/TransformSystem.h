#pragma once
#include "System.h"

#include "../Components/Transform.h"

class TransformSystem : public System<TransformSystem, Transform>
{
public:
	void initComponent(Transform& transform, glm::vec2 position, glm::vec2 size);
	void destroyComponent(Transform& transform);

	void translate(size_t entityID, glm::vec2 delta, size_t componentIndex = 0);
	void resize(size_t entityID, glm::vec2 delta, size_t componentIndex = 0);

	void setPosition(size_t entityID, glm::vec2 position, size_t componentIndex = 0);
	void setSize(size_t entityID, glm::vec2 size, size_t componentIndex = 0);
};