#include "DataTypes/Object.h"
#include "Engine/engine.h"

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

	bool Object::is_alive() 
	{
		return is_alive;
	}

	std::vector<Object>& Object::get_children()
	{
		return children;
	}

}
