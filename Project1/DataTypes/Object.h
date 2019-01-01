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

		void addChild(Object& obj);

		// helper functions
		uint64_t getId() const;
		void setId(const uint64_t _id);
		bool isAlive();

	private:

		uint64_t id;
		std::vector<Object> children;

		bool isAlive = true;
		bool isRenderable = false;
	};

}




