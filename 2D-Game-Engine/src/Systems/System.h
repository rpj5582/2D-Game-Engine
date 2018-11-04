#pragma once

template<typename SystemType, typename ComponentType>
class System
{
public:
	System();
	~System();

	static SystemType* getInstance();

	template<typename... Args>
	size_t addComponent(size_t entityID, Args... args);
	const ComponentType* getComponent(size_t entityID, size_t componentIndex = 0);
	const std::vector<const ComponentType*> getComponents(size_t entityID);
	bool removeComponent(size_t entityID, size_t componentIndex = 0);

protected:
	template<typename... Args>
	void initComponent(ComponentType& transform, Args... args);
	void destroyComponent(ComponentType& transform);

	ComponentType* getComponentNonConst(size_t entityID, size_t componentIndex);

	std::vector<ComponentType> m_components;

private:
	std::pair<typename std::vector<ComponentType>::iterator, typename std::vector<ComponentType>::iterator> getIterators(size_t entityID);

	static SystemType* m_instance;
};

template<typename SystemType, typename ComponentType>
inline System<SystemType, ComponentType>::System()
{
	if (!m_instance)
		m_instance = static_cast<SystemType*>(this);
}

template<typename SystemType, typename ComponentType>
inline System<SystemType, ComponentType>::~System()
{
	for (size_t i = 0; i < m_components.size(); i++)
	{
		destroyComponent(m_components[i]);
	}
	m_components.clear();
}

template<typename SystemType, typename ComponentType>
inline SystemType* System<SystemType, ComponentType>::getInstance()
{
	return m_instance;
}

template<typename SystemType, typename ComponentType>
template<typename... Args>
inline size_t System<SystemType, ComponentType>::addComponent(size_t entityID, Args... args)
{
	size_t count = std::count_if(m_components.begin(), m_components.end(), [&entityID](const ComponentType& component)
	{
		return component.entityID == entityID;
	});

	auto itPair = getIterators(entityID);
	auto it = itPair.second;
	it = m_components.emplace(it, entityID);

	ComponentType& component = *it;
	initComponent(component, args...);

	return count;
}

template<typename SystemType, typename ComponentType>
inline const ComponentType* System<SystemType, ComponentType>::getComponent(size_t entityID, size_t componentIndex)
{
	auto itPair = getIterators(entityID);
	auto it = itPair.first + componentIndex;

	if (it < itPair.second)
		return &(*it);

	return nullptr;
}

template<typename SystemType, typename ComponentType>
inline const std::vector<const ComponentType*> System<SystemType, ComponentType>::getComponents(size_t entityID)
{
	auto itPair = getIterators(entityID);

	if (itPair.first != m_components.end())
	{
		std::vector<const ComponentType*> components(std::distance(itPair.first, itPair.second));
		for (size_t i = 0; i < components.size(); i++)
		{
			components[i] = &(*(itPair.first + i));
		}

		return components;
	}
	else
	{
		std::vector<const ComponentType*> components;
		return components;
	}
}

template<typename SystemType, typename ComponentType>
inline bool System<SystemType, ComponentType>::removeComponent(size_t entityID, size_t componentIndex)
{
	auto itPair = getIterators(entityID);
	auto it = itPair.first + componentIndex;

	if (it < itPair.second)
	{
		destroyComponent(*it);
		m_components.erase(it);
		return true;
	}

	return false;
}

template<typename SystemType, typename ComponentType>
template<typename ...Args>
inline void System<SystemType, ComponentType>::initComponent(ComponentType& component, Args... args)
{
	static_cast<SystemType*>(this)->initComponent(component, args...);
}

template<typename SystemType, typename ComponentType>
inline void System<SystemType, ComponentType>::destroyComponent(ComponentType& component)
{
	static_cast<SystemType*>(this)->destroyComponent(component);
}

template<typename SystemType, typename ComponentType>
inline ComponentType* System<SystemType, ComponentType>::getComponentNonConst(size_t entityID, size_t componentIndex)
{
	auto itPair = getIterators(entityID);
	auto it = itPair.first + componentIndex;

	if (it < itPair.second)
		return &(*it);

	return nullptr;
}

template<typename SystemType, typename ComponentType>
inline std::pair<typename std::vector<ComponentType>::iterator, typename std::vector<ComponentType>::iterator> System<SystemType, ComponentType>::getIterators(size_t entityID)
{
	auto itPair = std::equal_range(m_components.begin(), m_components.end(), ComponentType(entityID), [](const ComponentType& component1, const ComponentType& component2)
	{
		return component1.entityID < component2.entityID;
	});

	return itPair;
}

template<typename SystemType, typename ComponentType>
SystemType* System<SystemType, ComponentType>::m_instance = nullptr;
