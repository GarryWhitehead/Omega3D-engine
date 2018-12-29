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
		return newObject;
	}

	void ObjectManager::destroyObject(Object& obj)
	{
		uint32_t id = obj.getId();
		objects.erase(obj);
		freeIds.push_front(id);
	}

	

}