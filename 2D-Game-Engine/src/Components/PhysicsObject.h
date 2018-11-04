#pragma once
#include "Component.h"

class b2Body;

struct PhysicsObject : public Component
{
	PhysicsObject(size_t entityID) : Component(entityID), body(nullptr) {}

	b2Body* body;
};