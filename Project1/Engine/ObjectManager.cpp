#include "Engine/ObjectManager.h"
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

		// set id and all managers that this entity is already registered with
		newObject.setId(id);
		RegisteredManager emptyManager;
		objects.insert(std::make_pair(newObject, emptyManager));
	}

	void ObjectManager::destroyObject(Object& obj)
	{
		uint32_t id = obj.getId();
		objects.erase(obj);
		freeIds.push_front(id);
	}

	bool ObjectManager::registerManager(Object& obj, uint32_t managerId, uint32_t objIndex)
	{
		if (objects.find(obj) == objects.end()) {
			return false;
		}

		RegisteredManager manager{ managerId, objIndex };
		auto iter = objects.find(obj);
		iter->second.push_back(manager);
		return true;
	}

	bool ObjectManager::registerManager(Object& obj, RegisteredManager manager)
	{
		if (objects.find(obj) == objects.end()) {
			return false;
		}

		auto iter = objects.find(obj);
		iter->second.push_back(manager);
		return true;
	}

	void ObjectManager::removeManager(Object& obj, uint32_t managerId)
	{
		if (objects.find(obj) != objects.end()) {
			
			auto iter = objects.find(obj);
			uint32_t count = 0;
			for (auto reg : iter->second) {
				if (reg.managerId == managerId) {
					iter->second.erase(iter->second.begin() + count);
				}
				++count;
			}
		}
	}

}