#pragma once

struct Component
{
	Component(size_t entityID) : entityID(entityID) {}

	const size_t entityID;

	void operator=(const Component& other) {}
};