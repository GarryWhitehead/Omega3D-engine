#pragma once

#include "omega-engine/Object.h"

#include <cstdint>

namespace OmegaEngine
{

class OEObject : public Object
{

public:
    OEObject(const uint64_t id);

    // operator overloads
    bool operator==(const OEObject& obj) const;

    // helper functions
    uint64_t getId() const;
    void setId(const uint64_t objId);
    bool isActive() const;

private:
    uint64_t id;
    bool active = true;
};

}