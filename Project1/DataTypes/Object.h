#pragma once
#include <stdint.h>
#include <unordered_map>

namespace OmegaEngine
{

	struct RegisteredManager
	{
		uint32_t managerId;
		uint32_t index;
	};

	class Object
	{
	public:

		Object();

		bool operator==(const Object& obj) const;

		// helper functions
		uint64_t getId() const { return id; }
		void setId(const uint64_t _id) { id = _id; }
		bool isAlive() const { return isAlive; }

	private:

		uint64_t id;

		bool isAlive;
	};

}




