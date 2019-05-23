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

	void Object::addChild(Object& obj)
	{
		obj.parentId = id;

		children.push_back(obj);
	}

	// helper functions
	uint64_t Object::getId() const
	{
		return id;
	}

	void Object::setId(const uint64_t _id)
	{
		id = _id;
	}

	uint64_t Object::getParent() const
	{
		return parentId;
	}
	Object& Object::getLastChild()
	{
		if (!children.empty()) {
			return children.back();
		}
		throw std::out_of_range("No children in parent object. This shouldn't happen!");
	}

	std::vector<Object>& Object::getChildren()
	{
		return children;
	}

}
