#pragma once
#include <vector>
#include <deque>

#include "DataTypes/Object.h"

#define MINIMUM_FREE_IDS 100

namespace OmegaEngine
{

	struct HashObject
	{
		size_t operator()(const Object& obj) const
		{
			return(std::hash<uint32_t>()(obj.getId()));
		}
	};

	class ObjectManager
	{
	public:

		ObjectManager();

		Object& createObject();
		void destroyObject(Object& obj);

		// registers a single manager with a entity
		bool registerManager(Object& obj, uint32_t managerId, uint32_t objIndex);
		bool registerManager(Object& obj, RegisteredManager manager);

		void removeManager(Object& obj, uint32_t managerId);

		// registers a list of managers with a entity
		template <typename... Ts>
		void registerManager(Object& obj, Ts... ts)
		{
			std::vector<ComponentDataType> = { ts... };
		}

	private:

		uint32_t nextId;

		// using a hash map here for faster look up for entites and their managers 
		// this is defines all the component manages that this entity is registered with and the index into each of their buffers
		// component manager are determined by unique ids.
		std::unordered_map<Object, std::vector<RegisteredManager, HashObject> > objects;
		std::deque<uint32_t> freeIds;
	};

}



