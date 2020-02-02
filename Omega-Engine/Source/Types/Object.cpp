#include "Object.h"

namespace OmegaEngine
{

OEObject::OEObject(const uint64_t id) : id(id)
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

void OEObject::setId(const uint64_t objId)
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

}