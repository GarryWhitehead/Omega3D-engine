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

	Object* ObjectManager::createObject()
	{
		Object newObject;

		uint32_t id = 0;
		if (!freeIds.empty() && freeIds.size() > MINIMUM_FREE_IDS) {

			id = freeIds.front();
			freeIds.pop_front();
		}
		else {
			id = ++nextId;
		}
		
		newObject.set_id(id);
		objects.insert(std::make_pair(id, newObject));
		return &objects[id];
	}

	Object* ObjectManager::createChildObject(Object& obj)
	{
		// create a new object as usual but not added to the list of active objects as this is a child object and is part of another 
		Object newObject;

		uint32_t id = 0;
		if (!freeIds.empty() && freeIds.size() > MINIMUM_FREE_IDS) {

			id = freeIds.front();
			freeIds.pop_front();
		}
		else {
			id = ++nextId;
		}

		newObject.set_id(id);
		obj.add_child(newObject);
		
		return &obj.get_last_child();
	}

	void ObjectManager::destroyObject(Object& obj)
	{
		uint64_t id = obj.get_id();
		objects.erase(id);
		freeIds.push_front(id);
	}

	

}