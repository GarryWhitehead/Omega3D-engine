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
			return(std::hash<uint32_t>()(obj.getId()));
		}
	};

	class ObjectManager
	{
	public:

		ObjectManager();

		Object& createObject();
		Object& createChildObject(Object& object);

		void destroyObject(Object& obj);

	private:

		uint32_t nextId;

		std::vector<Object> objects;
		std::deque<uint32_t> freeIds;
	};

}



