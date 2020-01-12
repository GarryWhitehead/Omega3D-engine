#pragma once

#include "omega-engine/ObjectManager.h"

#include "OEMaths/OEMaths.h"

#include "utility/GeneralUtil.h"
#include "utility/Logger.h"

#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>
#include <cstdint>

#define MINIMUM_FREE_IDS 100

namespace OmegaEngine
{
// forward declerations

using ObjectId = uint64_t;
using ObjHandle = uint64_t;

class OEObject : public Object
{

public:
    OEObject() = default;

    OEObject(const ObjectId id);

    // operator overloads
	bool operator==(const OEObject& obj) const;

    // helper functions
	uint64_t getId() const;
    void setId(const ObjectId objId);
    bool isActive() const;

private:

    uint64_t id;
    bool active = true;
};

class OEObjectManager : public ObjectManager
{
public:

	OEObjectManager() = default;

	// no copying or moving of this manager
	OEObjectManager(const OEObjectManager&) = delete;
	OEObjectManager& operator=(const OEObjectManager&) = delete;
	OEObjectManager(OEObjectManager&&) = delete;
	OEObjectManager& operator=(OEObjectManager&&) = delete;

	// object creation functions 
	OEObject* createObject();
	
	// 
	void destroyObject(OEObject* obj);

	std::vector<OEObject> &getObjectsList()
	{
		return objects;
	}

private:

	uint32_t nextId = 0;

	// this is an unordered map so we can quickly find objects based on their id. Saves having to iterate through a vector which
	// could be costly time-wise
	std::vector<OEObject> objects;

	// ids of objects which has been destroyed and can be re-used
	std::deque<uint32_t> freeIds;
};

} // namespace OmegaEngine
