#pragma once

#include "OEMaths/OEMaths.h"

#include "Types/Object.h"

#include "utility/GeneralUtil.h"
#include "utility/logger.h"

#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#define MINIMUM_FREE_IDS 100

namespace OmegaEngine
{
// forward declerations


class ObjectManager
{
public:

	ObjectManager() = default;
	~ObjectManager() = default;

	// no copying or moving of this manager
	ObjectManager(const ObjectManager&) = delete;
	ObjectManager& operator=(const ObjectManager&) = delete;
	ObjectManager(ObjectManager&&) = delete;
	ObjectManager& operator=(ObjectManager&&) = delete;

	// object creation functions 
	Object* createObject();
	
	// 
	void destroyObject(Object &obj);

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