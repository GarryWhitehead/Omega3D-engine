#include "ObjectInterface/ObjectManager.h"

#include "ObjectInterface/Object.h"

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

		Object& object = objects[id];

		object.setId(id);
		return &object;
	}

	Object* ObjectManager::createChildObject(Object& parentObj)
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

		return &parentObj.addChild(id);
	}

	void ObjectManager::destroyObject(Object& obj)
	{
		uint64_t id = obj.getId();
		objects.erase(id);
		freeIds.push_front(id);
	}

	// grouped object functions
	GroupedHandle ObjectManager::createGroupedObject(OEMaths::vec3f& translation, OEMaths::vec3f& scale, OEMaths::quatf& rotation)
	{
		groupedObjects.emplace_back(std::make_unique<GroupedObject>());
		return groupedObjects.size() - 1;
	}

	void ObjectManager::addObjectToGroup(const GroupedHandle handle, Object* object)
	{
		assert(handle < groupedObjects.size());
		assert(object != nullptr);
		groupedObjects[handle]->objects.emplace_back(object);
	}

}