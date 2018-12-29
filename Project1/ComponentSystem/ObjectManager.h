#pragma once
#include <vector>
#include <deque>
#include "Utility/GeneralUtil.h"
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

	private:

		uint32_t nextId;

		std::vector<Object> objects;
		std::deque<uint32_t> freeIds;
	};

}



