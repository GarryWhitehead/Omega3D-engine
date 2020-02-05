#pragma once

#include "Types/Object.h"

#include <unordered_map>
#include <vector>

namespace OmegaEngine
{

/**
	* Hasher overloads to allow for objects to be hashed via their id
	*/
struct ObjHash
{
	size_t operator()(const OEObject& id) const
	{
		return std::hash<uint64_t>{}(id.getId());
	}
};

struct ObjEqual
{
	bool operator()(const OEObject& lhs, const OEObject& rhs) const
	{
		return lhs.getId() == rhs.getId();
	}
};


class ComponentManager
{
public:
	ComponentManager() = default;
	virtual ~ComponentManager() = default;

	// component managers are neither copyable or moveable
	ComponentManager(const ComponentManager&) = delete;
	ComponentManager& operator=(const ComponentManager&) = delete;
	ComponentManager(ComponentManager&&) = delete;
	ComponentManager& operator=(ComponentManager&&) = delete;

	/**
	* @brief Adds an object to the list and returns its location
	* This will either be a new slot or an already created one if any
	* have been freed
	*/
	ObjectHandle addObject(OEObject& obj)
	{
		uint64_t retIdx = 0;

		// if theres no free slots, then create a new one
		if (freeSlots.empty())
		{
			objects.emplace(obj, index);
			retIdx = index++;
		}
		else
		{
			// otherwise, use an already used container slot
			retIdx = freeSlots.back();
			objects.emplace(obj, retIdx);
			freeSlots.pop_back();
		}
		return ObjectHandle(retIdx);
	}

	/**
	* @brief Returns an objects index value if found 
	* Note: returns zero if not found 
	*/
    ObjectHandle getObjIndex(OEObject& obj)
	{
		auto iter = objects.find(obj);
		if (iter == objects.end())
		{
			return ObjectHandle{UINT64_MAX}; //< zero indicates an error
		}
		return ObjectHandle(iter->second);
	}

	/**
	* @brief Removes an object from the manager and adds its slot index to
	* to the freed list for reuse.
	*/
	bool removeObject(OEObject& obj)
	{
		auto iter = objects.find(obj);
		if (iter == objects.end())
		{
			return false;
		}
		freeSlots.emplace_back(iter->second);
		objects.erase(iter);
        
        return true;
	}

protected:
	// the objects which contain this component and their index location
	std::unordered_map<OEObject, size_t, ObjHash, ObjEqual> objects;

	// free buffer indices from destroyed objects.
	// rather than resize buffers which will be slow, empty slots in manager containers
	// are stored here and re-used
	std::vector<size_t> freeSlots;

	// the current index into the main manager buffers which will be allocated
	// to the next object that is added.
	uint64_t index = 0;
};
}    // namespace OmegaEngine
