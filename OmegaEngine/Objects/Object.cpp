#include "Objects/Object.h"
#include "Engine/engine.h"

#include <cstdint>

namespace OmegaEngine
{
	Object::Object()
	{
	}

	Object::~Object()
	{
	}

	bool Object::operator==(const Object& obj) const
	{
		return id == obj.id;
	}

	void Object::add_child(Object& obj)
	{
		obj.parent_id = id;

		children.push_back(obj);
	}

	// helper functions
	uint64_t Object::get_id() const
	{
		return id;
	}

	void Object::set_id(const uint64_t _id)
	{
		id = _id;
	}

	uint64_t Object::get_parent() const
	{
		return parent_id;
	}
	Object& Object::get_last_child()
	{
		if (!children.empty()) {
			return children.back();
		}
		throw std::out_of_range("No children in parent object. This shouldn't happen!");
	}

	std::vector<Object>& Object::get_children()
	{
		return children;
	}

}
