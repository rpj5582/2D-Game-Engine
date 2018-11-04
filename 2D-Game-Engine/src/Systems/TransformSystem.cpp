#include "stdafx.h"
#include "TransformSystem.h"

void TransformSystem::translate(size_t entityID, glm::vec2 delta, size_t componentIndex)
{
	Transform* transform = getComponentNonConst(entityID, componentIndex);
	if(transform)
	{
		transform->position += delta;
	}
	
}

void TransformSystem::resize(size_t entityID, glm::vec2 delta, size_t componentIndex)
{
	Transform* transform = getComponentNonConst(entityID, componentIndex);
	if (transform)
	{
		transform->size += delta;
	}
}

void TransformSystem::setPosition(size_t entityID, glm::vec2 position, size_t componentIndex)
{
	Transform* transform = getComponentNonConst(entityID, componentIndex);
	if (transform)
	{
		transform->position = position;
	}
}

void TransformSystem::setSize(size_t entityID, glm::vec2 size, size_t componentIndex)
{
	Transform* transform = getComponentNonConst(entityID, componentIndex);
	if (transform)
	{
		transform->size = size;
	}
}

void TransformSystem::initComponent(Transform& transform, glm::vec2 position, glm::vec2 size)
{
	transform.position = position;
	transform.size = size;
}

void TransformSystem::destroyComponent(Transform& transform)
{
}
