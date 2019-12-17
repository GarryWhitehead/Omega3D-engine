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
	size_t operator()(Object const& id) const noexcept
	{
		return std::hash<uint64_t>{}(id.getId());
	}
};

struct ObjEqual
{
	bool operator()(const Object& lhs, const Object& rhs) const
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
	size_t addObject(Object& obj)
	{
		size_t retIdx;

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
		return index;
	}

	/**
	* @brief Returns an objects index value if found 
	* Note: returns zero if not found 
	*/
	size_t getObjIndex(Object& obj)
	{
		auto iter = objects.find(obj);
		if (iter == objects.end())
		{
			return 0;	//< zero indicates an error
		}
		return iter->second;
	}

	/**
	* @brief Removes an object from the manager and adds its slot index to
	* to the freed list for reuse.
	*/
	bool removeObject(Object& obj)
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
	std::unordered_map<Object, size_t, ObjHash, ObjEqual> objects;

	// free buffer indices from destroyed objects.
	// rather than resize buffers which will be slow, empty slots in manager containers
	// are stored here and re-used
	std::vector<size_t> freeSlots;

	// the current index into the main manager buffers which will be allocated
	// to the next object that is added. The index starts from one as zero is reserved
	// as primarily a error indicator (though can be used for other things)
	size_t index = 1;
};
}    // namespace OmegaEngine
