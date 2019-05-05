#pragma once

#include "Utility/GeneralUtil.h"
#include "Utility/logger.h"
#include "Objects/Object.h"

#include <vector>
#include <deque>
#include <memory>
#include <unordered_map>

#define MINIMUM_FREE_IDS 100

namespace OmegaEngine
{
	// forward declerations

	struct HashObject
	{
		size_t operator()(const Object& obj) const
		{
			return(std::hash<uint64_t>()(obj.get_id()));
		}
	};

	class ObjectManager
	{
	public:

		ObjectManager();
		~ObjectManager();

		Object* createObject();
		Object* createChildObject(Object& object);

		void destroyObject(Object& obj);

		Object* get_object_recursive(uint64_t id, Object& parent)
		{
			Object* obj = nullptr;
			if (parent.get_id() == id) {
				return &parent;
			}

			auto& children = parent.get_children();
			for (auto& child : children) {
				obj = get_object_recursive(id, child);
				if (obj) {
					break;
				}
			}

			return obj;
		}

		Object* get_object(uint64_t id)
		{
			Object* required_obj = nullptr;
			for (auto& obj : objects) {

				required_obj = get_object_recursive(id, obj.second);
				if (required_obj) {
					break;
				}
			}

			if (!required_obj) {
				LOGGER_ERROR("Fatal Error. Unable to find object with index %I64i.\n", id);
			}

			return required_obj;
		}

		std::unordered_map<uint64_t, Object>& get_objects_list()
		{
			return objects;
		}

	private:

		uint32_t nextId = 0;

		// this is an unordered map so we can quickly find objects based on their id. Saves having to iterate through a vector which
		// could be costly time-wise
		std::unordered_map<uint64_t, Object> objects;

		std::deque<uint32_t> freeIds;
	};

}



