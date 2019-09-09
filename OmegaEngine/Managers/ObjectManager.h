#pragma once

#include "OEMaths/OEMaths.h"
#include "ObjectInterface/Object.h"
#include "Utility/GeneralUtil.h"
#include "Utility/logger.h"

#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#define MINIMUM_FREE_IDS 100

namespace OmegaEngine
{
// forward declerations

using GroupedHandle = uint64_t;

class ObjectManager
{
public:

	ObjectManager();
	~ObjectManager();

	// object creation functions 
	Object& createObject();
	Object& createChildObject(Object &parentObj);
	
	// 
	void destroyObject(Object &obj);

	// templated functions
	Object *getObjectRecursive(uint64_t id, Object &parent)
	{
		Object *obj = nullptr;
		if (parent.getId() == id)
		{
			return &parent;
		}

		auto &children = parent.getChildren();
		for (auto &child : children)
		{
			obj = getObjectRecursive(id, child);
			if (obj)
			{
				break;
			}
		}

		return obj;
	}

	Object *getObject(uint64_t id)
	{
		Object *requiredObj = nullptr;
		for (auto &obj : objects)
		{
			requiredObj = getObjectRecursive(id, obj.second);
			if (requiredObj)
			{
				break;
			}
		}

		if (!requiredObj)
		{
			LOGGER_ERROR("Fatal Error. Unable to find object with index %I64i.\n", id);
		}

		return requiredObj;
	}

	std::unordered_map<uint64_t, Object> &getObjectsList()
	{
		return objects;
	}

private:

	uint32_t nextId = 0;

	// this is an unordered map so we can quickly find objects based on their id. Saves having to iterate through a vector which
	// could be costly time-wise
	std::unordered_map<uint64_t, Object> objects;

	// ids of objects which has been destroyed and can be re-used
	std::deque<uint32_t> freeIds;
};

} // namespace OmegaEngine
