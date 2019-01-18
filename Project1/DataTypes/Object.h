#pragma once

#include "Utility/GeneralUtil.h"

#include <stdint.h>
#include <unordered_map>

namespace OmegaEngine
{

	class Object
	{
	public:

		struct HashObject
		{
			size_t operator()(const Object& obj) const
			{
				return(std::hash<uint32_t>()(obj.get_id()));
			}
		};

		struct ComponentManagerInfo
		{
			uint32_t managerId;
			uint32_t index;
		};

		using ComponentManagerId = uint32_t;

		Object();

		// operator overloads
		bool operator==(const Object& obj) const;

		void add_child(Object& obj);

		// helper functions
		uint64_t get_id() const;
		void set_id(const uint64_t _id);
		uint64_t get_parent() const;
		bool is_alive();

		std::vector<Object>& get_children();

	private:

		uint64_t id;
		uint64_t parent_id;
		std::vector<Object> children;

		bool isAlive = true;
	};

}




