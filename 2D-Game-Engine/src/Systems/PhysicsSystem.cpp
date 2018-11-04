#include "stdafx.h"
#include "PhysicsSystem.h"

#include "TransformSystem.h"

#include <Box2D.h>

void PhysicsContactListener::registerContactCallbacks(size_t entityID, ContactCallbackPair& callback)
{
	m_contactCallbackMap[entityID].push_back(callback);
}

void PhysicsContactListener::BeginContact(b2Contact* contact)
{
	b2Fixture* fixtureA = contact->GetFixtureA();
	b2Fixture* fixtureB = contact->GetFixtureB();

	size_t entityIDA = reinterpret_cast<size_t>(fixtureA->GetUserData());
	size_t entityIDB = reinterpret_cast<size_t>(fixtureB->GetUserData());

	const PhysicsObject* physicsObjectA = nullptr;
	const PhysicsObject* physicsObjectB = nullptr;

	const std::vector<const PhysicsObject*> rigidbodiesA = PhysicsSystem::getInstance()->getComponents(entityIDA);
	const std::vector<const PhysicsObject*> rigidbodiesB = PhysicsSystem::getInstance()->getComponents(entityIDB);

	for (size_t i = 0; i < rigidbodiesA.size(); i++)
	{
		if (rigidbodiesA[i]->body == fixtureA->GetBody())
		{
			physicsObjectA = rigidbodiesA[i];
			break;
		}
	}

	for (size_t i = 0; i < rigidbodiesB.size(); i++)
	{
		if (rigidbodiesB[i]->body == fixtureB->GetBody())
		{
			physicsObjectB = rigidbodiesB[i];
			break;
		}
	}

	if (!physicsObjectA || !physicsObjectB) return;

	if (m_contactCallbackMap.find(entityIDA) != m_contactCallbackMap.end())
	{
		const std::vector<ContactCallbackPair>& callbacks = m_contactCallbackMap[entityIDA];
		for (size_t i = 0; i < callbacks.size(); i++)
		{
			callbacks[i].first(physicsObjectA, physicsObjectB);
		}
	}

	if (m_contactCallbackMap.find(entityIDB) != m_contactCallbackMap.end())
	{
		const std::vector<ContactCallbackPair>& callbacks = m_contactCallbackMap[entityIDB];
		for (size_t i = 0; i < callbacks.size(); i++)
		{
			callbacks[i].first(physicsObjectB, physicsObjectA);
		}
	}
}

void PhysicsContactListener::EndContact(b2Contact* contact)
{
	b2Fixture* fixtureA = contact->GetFixtureA();
	b2Fixture* fixtureB = contact->GetFixtureB();

	size_t entityIDA = reinterpret_cast<size_t>(fixtureA->GetUserData());
	size_t entityIDB = reinterpret_cast<size_t>(fixtureB->GetUserData());

	const PhysicsObject* physicsObjectA = nullptr;
	const PhysicsObject* physicsObjectB = nullptr;

	const std::vector<const PhysicsObject*> rigidbodiesA = PhysicsSystem::getInstance()->getComponents(entityIDA);
	const std::vector<const PhysicsObject*> rigidbodiesB = PhysicsSystem::getInstance()->getComponents(entityIDB);

	for (size_t i = 0; i < rigidbodiesA.size(); i++)
	{
		if (rigidbodiesA[i]->body == fixtureA->GetBody())
		{
			physicsObjectA = rigidbodiesA[i];
			break;
		}
	}

	for (size_t i = 0; i < rigidbodiesB.size(); i++)
	{
		if (rigidbodiesB[i]->body == fixtureB->GetBody())
		{
			physicsObjectB = rigidbodiesB[i];
			break;
		}
	}

	if (!physicsObjectA || !physicsObjectB) return;

	if (m_contactCallbackMap.find(entityIDA) != m_contactCallbackMap.end())
	{
		const std::vector<ContactCallbackPair>& callbacks = m_contactCallbackMap[entityIDA];
		for (size_t i = 0; i < callbacks.size(); i++)
		{
			callbacks[i].second(physicsObjectA, physicsObjectB);
		}
	}

	if (m_contactCallbackMap.find(entityIDB) != m_contactCallbackMap.end())
	{
		const std::vector<ContactCallbackPair>& callbacks = m_contactCallbackMap[entityIDB];
		for (size_t i = 0; i < callbacks.size(); i++)
		{
			callbacks[i].second(physicsObjectB, physicsObjectA);
		}
	}
}

PhysicsSystem::PhysicsSystem()
{
	m_physicsWorld = new b2World({ 0, PHYSICS_GRAVITY });
	m_physicsWorld->SetContactListener(&m_contactListener);
}

PhysicsSystem::~PhysicsSystem()
{
	delete m_physicsWorld;
	m_physicsWorld = nullptr;
}

void PhysicsSystem::initComponent(PhysicsObject& physicsObject, glm::vec2 position, glm::vec2 collisionSize, b2BodyType bodyType, bool active)
{
	b2Vec2 scaledPosition = b2Vec2(position.x / PHYSICS_PIXELS_PER_METER, position.y / PHYSICS_PIXELS_PER_METER);
	b2Vec2 scaledDimensions = b2Vec2(collisionSize.x / PHYSICS_PIXELS_PER_METER, collisionSize.y / PHYSICS_PIXELS_PER_METER);

	b2BodyDef bodyDef;
	bodyDef.position = scaledPosition;
	bodyDef.type = bodyType;
	bodyDef.fixedRotation = true;
	bodyDef.linearDamping = 1.75f;
	bodyDef.active = active;

	physicsObject.body = m_physicsWorld->CreateBody(&bodyDef);
}

void PhysicsSystem::destroyComponent(PhysicsObject& physicsObject)
{
	if (m_physicsWorld && physicsObject.body)
		m_physicsWorld->DestroyBody(physicsObject.body);
}

void PhysicsSystem::addFixture(size_t entityID, size_t componentIndex, glm::vec2 offset, glm::vec2 collisionSize,
	bool isSensor, ContactCallback beginContactCallback, ContactCallback endContactCallback)
{
	const PhysicsObject* physicsObject = PhysicsSystem::getInstance()->getComponent(entityID, componentIndex);
	if (physicsObject)
	{
		b2Vec2 scaledPosition = b2Vec2(offset.x / PHYSICS_PIXELS_PER_METER, offset.y / PHYSICS_PIXELS_PER_METER);
		b2Vec2 scaledDimensions = b2Vec2(collisionSize.x / PHYSICS_PIXELS_PER_METER, collisionSize.y / PHYSICS_PIXELS_PER_METER);

		b2PolygonShape shape;
		shape.SetAsBox(scaledDimensions.x / 2, scaledDimensions.y / 2, scaledPosition, 0);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &shape;
		fixtureDef.density = 1.0f;
		fixtureDef.userData = (void*)entityID;
		fixtureDef.isSensor = isSensor;

		if (beginContactCallback && endContactCallback)
		{
			ContactCallbackPair callbackPair = std::make_pair(beginContactCallback, endContactCallback);
			m_contactListener.registerContactCallbacks(entityID, callbackPair);
		}

		physicsObject->body->CreateFixture(&fixtureDef);
	}
}

b2World& PhysicsSystem::getWorld() const
{
	return *m_physicsWorld;
}

void PhysicsSystem::applyImpulse(size_t entityID, glm::vec2 force, size_t componentIndex)
{
	PhysicsObject* physicsObject = getComponentNonConst(entityID, componentIndex);
	if (physicsObject)
	{
		physicsObject->body->ApplyLinearImpulseToCenter({ force.x / PHYSICS_PIXELS_PER_METER, force.y / PHYSICS_PIXELS_PER_METER }, true);
	}
}

void PhysicsSystem::update()
{
	m_physicsWorld->Step(PHYSICS_TIMESTEP, 8, 3);

	TransformSystem* transformSystem = TransformSystem::getInstance();
	for (size_t i = 0; i < m_components.size(); i++)
	{
		const PhysicsObject& physicsObject = m_components[i];
		b2Vec2 position = physicsObject.body->GetPosition();

		transformSystem->setPosition(physicsObject.entityID, glm::vec2(position.x * PHYSICS_PIXELS_PER_METER, position.y * PHYSICS_PIXELS_PER_METER));
	}
}

#ifdef _DEBUG
void PhysicsSystem::setDebugDraw(b2Draw* debugDraw)
{
	m_physicsWorld->SetDebugDraw(debugDraw);
}

void PhysicsSystem::drawDebugData()
{
	m_physicsWorld->DrawDebugData();
}
#endif
