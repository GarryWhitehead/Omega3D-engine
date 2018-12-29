#include "ComponentSystem/ObjectManager.h"
#include "DataTypes/Object.h"

namespace OmegaEngine
{

	ObjectManager::ObjectManager()
	{
	}

	Object& ObjectManager::createObject()
	{
		Object newObject;

		uint32_t id = 0;
		if (freeIds.size() > MINIMUM_FREE_IDS) {

			id = freeIds.front();
			freeIds.pop_front();
		}
		else {
			id = ++nextId;
		}
		
		newObject.setId(id);
		objects.push_back(newObject);
		return newObject;
	}

	Object& ObjectManager::createChildObject(Object& obj)
	{
		// create a new object as usual but not added to the list of active objects as this is a child object and is part of another 
		Object newObject;

		uint32_t id = 0;
		if (freeIds.size() > MINIMUM_FREE_IDS) {

			id = freeIds.front();
			freeIds.pop_front();
		}
		else {
			id = ++nextId;
		}

		newObject.setId(id);
		obj.addChild(newObject);

		return newObject;
	}

	void ObjectManager::destroyObject(Object& obj)
	{
		uint32_t id = obj.getId();
		objects.erase(obj);
		freeIds.push_front(id);
	}

	

}