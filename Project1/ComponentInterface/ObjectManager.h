#pragma once
#include <vector>
#include <deque>
#include <memory>

#include "Utility/GeneralUtil.h"
#include "DataTypes/Object.h"

#define MINIMUM_FREE_IDS 100

namespace OmegaEngine
{
	// forward declerations

	struct HashObject
	{
		size_t operator()(const Object& obj) const
		{
			return(std::hash<uint32_t>()(obj.get_id()));
		}
	};

	class ObjectManager
	{
	public:

		ObjectManager();

		Object& createObject();
		Object& createChildObject(Object& object);

		void destroyObject(Object& obj);

		Object& get_obj(uint64_t index)
		{
			if (objects.find(index) == objects.end()) {
				return {};
			}

			return objects[index];
		}

	private:

		uint32_t nextId;

		// this is an unordered map so we can quickly find objects based on their id. Saves having to iterate through a vector which
		// could be costly time-wise
		std::unordered_map<uint64_t, Object> objects;

		std::deque<uint32_t> freeIds;
	};

}



