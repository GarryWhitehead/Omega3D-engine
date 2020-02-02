#ifndef OBJECTMANAGER_HPP
#define OBJECTMANAGER_HPP

#include "utility/Compiler.h"

#include <cstdint>

namespace OmegaEngine
{

class OE_PUBLIC Object
{
public:

	Object() = default;

	uint64_t getId() const;

	bool isActive() const;

private:

};

}    // namespace OmegaEngine

#endif /* ObjectManager_hpp */
