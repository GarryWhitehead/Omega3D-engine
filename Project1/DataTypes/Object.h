#pragma once

#include "Utility/GeneralUtil.h"

#include <stdint.h>
#include <unordered_map>

namespace OmegaEngine
{

	class Object
	{
	public:

		struct ComponentManagerInfo
		{
			uint32_t managerId;
			uint32_t index;
		};

		using ComponentManagerId = uint32_t;

		Object();

		// operator overloads
		bool operator==(const Object& obj) const;

		template <typename T>
		void addComponent(uint32_t index)
		{
			uint32_t id = Util::event_type_id<T>();
			components[id].push_back(index);
		}

		void addChild(Object& obj);

		// helper functions
		uint64_t getId() const;
		void setId(const uint64_t _id);
		bool isAlive();

	private:

		uint64_t id;
		std::vector<Object> children;

		// a list of all the indicies that this object is registered with for a particular manager. The idea is that pointers to components aren't used so cache performance should be improved
		std::unordered_map<ComponentManagerId, std::vector<uint32_t> > components;

		bool isAlive = true;
		bool isRenderable = false;
	};

}




