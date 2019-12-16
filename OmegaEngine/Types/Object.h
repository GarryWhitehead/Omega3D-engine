#pragma once

#include <stdint.h>

namespace OmegaEngine
{

using ObjectId = uint64_t;

class Object
{

public:
	Object() = default;
	~Object() = default;

	Object(const ObjectId id)
	    : id(id)
	{
	}

	// operator overloads
	bool operator==(const Object& obj) const
	{
		return id == obj.id;
	}

	// helper functions
	ObjectId getId() const
	{
		return id;
	}

	void setId(const ObjectId objId)
	{
		id = objId;
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
