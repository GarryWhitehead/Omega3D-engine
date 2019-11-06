#include "ObjectManager.h"

#include "Types/Object.h"

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
	ObjectId id = 0;
	if (!freeIds.empty() && freeIds.size() > MINIMUM_FREE_IDS)
	{
		id = freeIds.front();
		freeIds.pop_front();
	}
	else
	{
		id = nextId++;
	}

	Object& object = objects[id];

	object.setId(id);
	return &object;
}

void ObjectManager::destroyObject(Object& obj)
{
	uint64_t id = obj.getId();
	objects.erase(id);
	freeIds.push_front(id);
}

}    // namespace OmegaEngine