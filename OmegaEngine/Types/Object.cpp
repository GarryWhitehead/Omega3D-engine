#include "Object.h"

#include "Core/engine.h"

#include <cstdint>

namespace OmegaEngine
{
Object::Object()
{
}

Object::Object(const uint32_t _id)
    : id(_id)
{
}

Object::~Object()
{
}

bool Object::operator==(const Object& obj) const
{
	return this->id == obj.id;
}

Object& Object::addChild(const uint32_t id)
{
	Object childObject(id);
	childObject.parentId = this->id;

	children.push_back(childObject);

	return children[children.size() - 1];
}

// helper functions
uint64_t Object::getId() const
{
	return this->id;
}

void Object::setId(const uint64_t id)
{
	this->id = id;
}

uint64_t Object::getParent() const
{
	return this->parentId;
}

std::vector<Object>& Object::getChildren()
{
	return children;
}

}    // namespace OmegaEngine
