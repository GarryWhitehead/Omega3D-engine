#pragma once

#include <stdint.h>

namespace OmegaEngine
{
struct ComponentBase;

class Object
{

public:

	using ObjectId = uint64_t;

	Object() = default;
	~Object() = default;

	Object(const ObjectId id)
	{
		this->id = id;
	}

	// object are not copyable - could mess up ids
	Object(const Object&) = delete;
	Object& operator=(const Object&) = delete;

	// but are moveable
	Object(Object&&) = default;
	Object& operator=(Object&&) = default;

	// operator overloads
	bool operator==(const Object &obj) const
	{
		return this->id == obj.id;
	}

	// helper functions
	ObjectId getId() const
	{
		return this->id;
	}

private:

	ObjectId id;

};

} // namespace OmegaEngine
