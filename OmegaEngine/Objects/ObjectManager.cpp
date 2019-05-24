#include "Objects/ObjectManager.h"

#include "Objects/Object.h"

namespace OmegaEngine
{

	ObjectManager::ObjectManager()
	{
	}

	ObjectManager::~ObjectManager()
	{
	}

	Object ObjectManager::createObject()
	{
		uint32_t id = 0;
		if (!freeIds.empty() && freeIds.size() > MINIMUM_FREE_IDS) 
		{
			id = freeIds.front();
			freeIds.pop_front();
		}
		else 
		{
			id = nextId++;
		}
		
		nextObject.setId(id);
		objects.emplace(id, nextObject);
		
		return nextObject;
	}

	Object ObjectManager::createChildObject(Object& parentObj)
	{
		uint32_t id = 0;
		if (!freeIds.empty() && freeIds.size() > MINIMUM_FREE_IDS)
		{
			id = freeIds.front();
			freeIds.pop_front();
		}
		else 
		{
			id = nextId++;
		}

		nextObject.setId(id);
		parentObj.addChild(nextObject);

		return nextObject;
	}

	void ObjectManager::destroyObject(Object& obj)
	{
		uint64_t id = obj.getId();
		objects.erase(id);
		freeIds.push_front(id);
	}

	

}