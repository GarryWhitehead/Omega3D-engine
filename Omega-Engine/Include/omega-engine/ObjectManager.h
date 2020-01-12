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


class OE_PUBLIC ObjectManager
{
public:
	ObjectManager() = default;

	// object creation functions
	Object* createObject();

	//
	void destroyObject(Object* obj);

private:
};

}    // namespace OmegaEngine

#endif /* ObjectManager_hpp */
