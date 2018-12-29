#include "DataTypes/Object.h"
#include "Engine/engine.h"

namespace OmegaEngine
{
	Object::Object()
	{
	}

	bool Object::operator==(const Object& obj) const
	{
		return id == obj.id;
	}

	void Object::addChild(Object& obj)
	{
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

	bool Object::isAlive() 
	{
		return isAlive;
	}

}
