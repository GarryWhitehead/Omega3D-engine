#pragma once

#include "Utility/GeneralUtil.h"
#include "Utility/logger.h"
#include "ObjectInterface/Object.h"
#include "OEMaths/OEMaths.h"

#include <vector>
#include <deque>
#include <memory>
#include <unordered_map>

#define MINIMUM_FREE_IDS 100

namespace OmegaEngine
{
	// forward declerations

	using GroupedHandle = uint64_t;

	class ObjectManager
	{
	public:

		struct GroupedObject
		{
			// this need some work - what if objects are destroyed but remain here?
			std::vector<Object*> objects;

			// world transform for all grouped objects
			OEMaths::vec3f translation;
			OEMaths::vec3f scale;
			OEMaths::quatf rotation;
		};

		ObjectManager();
		~ObjectManager();

		// single objects
		Object* createObject();
		Object* createChildObject(Object& parentObj);
		void destroyObject(Object& obj);

		// grouped objects
		GroupedHandle createGroupedObject(OEMaths::vec3f& translation, OEMaths::vec3f& scale, OEMaths::quatf& rotation);
		void addObjectToGroup(const GroupedHandle handle, Object* object);

		// templated functions
		Object* getObjectRecursive(uint64_t id, Object& parent)
		{
			Object* obj = nullptr;
			if (parent.getId() == id) 
			{
				return &parent;
			}

			auto& children = parent.getChildren();
			for (auto& child : children) 
			{
				obj = getObjectRecursive(id, child);
				if (obj) 
				{
					break;
				}
			}

			return obj;
		}

		Object* getObject(uint64_t id)
		{
			Object* requiredObj = nullptr;
			for (auto& obj : objects) 
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

		std::unordered_map<uint64_t, Object>& getObjectsList()
		{
			return objects;
		}

	private:

		uint32_t nextId = 0;

		// this is an unordered map so we can quickly find objects based on their id. Saves having to iterate through a vector which
		// could be costly time-wise
		std::unordered_map<uint64_t, Object> objects;

		std::vector<std::unique_ptr<GroupedObject> > groupedObjects;

		// ids of objects which has been destroyed and can be re-used
		std::deque<uint32_t> freeIds;
	};

}



