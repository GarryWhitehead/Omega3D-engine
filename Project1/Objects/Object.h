#pragma once

#include "Utility/GeneralUtil.h"

#include <stdint.h>
#include <unordered_map>

namespace OmegaEngine
{

	class Object
	{

	public:

		using ManagerId = uint32_t;
		using ManagerIndex = uint32_t;

		Object();
		~Object();

		// operator overloads
		bool operator==(const Object& obj) const;

		void add_child(Object& obj);

		// helper functions
		uint64_t get_id() const;
		void set_id(const uint64_t _id);
		uint64_t get_parent() const;
		bool is_alive() const;

		template <typename T>
		uint32_t get_manager_index()
		{
			uint32_t man_id = Util::event_type_id<T>();
			assert(components.find(man_id) != components.end);
			return components[man_id];
		}

		template <typename T>
		uint32_t add_manager(uint32_t index)
		{
			uint32_t man_id = Util::event_type_id<T>();
			components[man_id] = index;
		}

		template <typename T>
		bool hasComponent()
		{
			uint32_t man_id = Util::event_type_id<T>();
			if (components.find(man_id) == components.end()) {
				return false;
			}
			return true;
		}

		std::vector<Object>& get_children();

	private:

		uint64_t id;
		uint64_t parent_id;
		std::vector<Object> children;

		std::unordered_map<ManagerId, ManagerIndex> components;

		bool isAlive = true;
	};

}




