#include "ObjectManager.h"

namespace OmegaEngine
{

// ================================================================================================================

OEObject::OEObject(const ObjectId id)
    : id(id)
{
}

// operator overloads
bool OEObject::operator==(const OEObject& obj) const
{
	return id == obj.id;
}

// helper functions
uint64_t OEObject::getId() const
{
	return id;
}

void OEObject::setId(const ObjectId objId)
{
	id = objId;
}

inline bool OEObject::isActive() const
{
	return active;
}

// ================= front-end ===========================================

uint64_t Object::getId() const
{
	return static_cast<const OEObject*>(this)->getId();
}

bool Object::isActive() const
{
	return static_cast<const OEObject*>(this)->isActive();
}

// ===============================================================================================================

OEObject* OEObjectManager::createObject()
{
	ObjectId id = 0;
	if (!freeIds.empty() && freeIds.size() > MINIMUM_FREE_IDS)
	{
		id = freeIds.front();
		freeIds.pop_front();
	}
	else
	{
		id = nextId++;
	}

	OEObject object(id);
	objects.emplace_back(object);
	return &objects.back();
}

void OEObjectManager::destroyObject(OEObject* obj)
{
	size_t count = 0;
	for (auto& object : objects)
	{
		if (*obj == object)
		{
			break;
		}
		++count;
	}
	// completley remove from the list - costly!
	objects.erase(objects.begin() + count);
	freeIds.push_front(obj->getId());
}

// ===================== front-end =====================================

Object* ObjectManager::createObject()
{
	return static_cast<OEObjectManager*>(this)->createObject();
}

void ObjectManager::destroyObject(Object* obj)
{
	static_cast<OEObjectManager*>(this)->destroyObject(static_cast<OEObject*>(obj));
}

}    // namespace OmegaEngine
