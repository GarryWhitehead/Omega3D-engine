#pragma once

#include <stdint.h>

namespace OmegaEngine
{
struct ComponentBase;

using ObjectId = uint64_t;

class Object
{

public:
	Object() = default;
	~Object() = default;

	Object(const ObjectId id)
	{
		this->id = id;
	}

	// object are copyable
	Object(const Object&) = default;
	Object& operator=(const Object&) = default;

	// and moveable
	Object(Object&&) = default;
	Object& operator=(Object&&) = default;

	// operator overloads
	bool operator==(const Object& obj) const
	{
		return this->id == obj.id;
	}

	// helper functions
	ObjectId getId() const
	{
		return this->id;
	}

	void setId(const ObjectId id)
	{
		this->id = id;
	}

	inline bool isActive() const
	{
		return active;
	}

private:
	ObjectId id;
	bool active = true;
};

}    // namespace OmegaEngine
