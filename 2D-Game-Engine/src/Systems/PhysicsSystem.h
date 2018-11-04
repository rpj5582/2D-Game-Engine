#pragma once
#include "System.h"

#include "../Components/PhysicsObject.h"

#ifdef _DEBUG
#include "../Debug/DebugDrawPhysics.h"
#endif

#include <Box2D.h>

#include <functional>

#define PHYSICS_TIMESTEP 0.01666666754f

#ifndef PHYSICS_PIXELS_PER_METER
	#define PHYSICS_PIXELS_PER_METER 16
#endif

#define PHYSICS_GRAVITY -PHYSICS_PIXELS_PER_METER * 4.0f

typedef std::function<void(const PhysicsObject*, const PhysicsObject*)> ContactCallback;
typedef std::pair<ContactCallback, ContactCallback> ContactCallbackPair;

class PhysicsContactListener : public b2ContactListener
{
public:
	void registerContactCallbacks(size_t entityID, ContactCallbackPair& callbacks);

	void BeginContact(b2Contact* contact) override;
	void EndContact(b2Contact* contact) override;

private:
	std::unordered_map<size_t, std::vector<ContactCallbackPair>> m_contactCallbackMap;
};

class PhysicsSystem : public System<PhysicsSystem, PhysicsObject>
{
public:
	PhysicsSystem();
	~PhysicsSystem();

	void initComponent(PhysicsObject& physicsObject, glm::vec2 position, glm::vec2 collisionSize, b2BodyType bodyType, bool active = true);
	void destroyComponent(PhysicsObject& physicsObject);

	void addFixture(size_t entityID, size_t componentIndex, glm::vec2 offset, glm::vec2 collisionSize, bool isSensor = false,
		ContactCallback beginContactCallback = nullptr, ContactCallback endContactCallback = nullptr);

	b2World& getWorld() const;

	void applyImpulse(size_t entityID, glm::vec2 force, size_t componentIndex = 0);

	void update();

#ifdef _DEBUG
	void setDebugDraw(b2Draw* debugDraw);
	void drawDebugData();
#endif

private:
	b2World* m_physicsWorld;
	PhysicsContactListener m_contactListener;
};
