#pragma once

#include "omega-engine/ObjectManager.h"

#include "OEMaths/OEMaths.h"

#include "utility/GeneralUtil.h"
#include "utility/Logger.h"

#include <deque>
#include <memory>
#include <unordered_map>
#include <vector>

#define MINIMUM_FREE_IDS 100

namespace OmegaEngine
{
// forward declerations

using ObjectId = uint64_t;
using ObjHandle = uint64_t;

class Object
{

public:
    Object() = default;
    ~Object() = default;

    Object(const ObjectId id)
        : id(id)
    {
    }

    // operator overloads
    bool operator==(const Object& obj) const
    {
        return id == obj.id;
    }

    // helper functions
    ObjectId getId() const
    {
        return id;
    }

    void setId(const ObjectId objId)
    {
        id = objId;
    }

    inline bool isActive() const
    {
        return active;
    }

private:
    ObjectId id;
    bool active = true;
};

class OEObjectManager : public ObjectManager
{
public:

	OEObjectManager() = default;
	~OEObjectManager();

	// no copying or moving of this manager
	OEObjectManager(const OEObjectManager&) = delete;
	OEObjectManager& operator=(const OEObjectManager&) = delete;
	OEObjectManager(OEObjectManager&&) = delete;
	OEObjectManager& operator=(OEObjectManager&&) = delete;

	// object creation functions 
	Object* createObject();
	
	// 
	void destroyObject(Object &obj);

	std::vector<Object> &getObjectsList()
	{
		return objects;
	}

private:

	uint32_t nextId = 0;

	// this is an unordered map so we can quickly find objects based on their id. Saves having to iterate through a vector which
	// could be costly time-wise
	std::vector<Object> objects;

	// ids of objects which has been destroyed and can be re-used
	std::deque<uint32_t> freeIds;
};

} // namespace OmegaEngine
