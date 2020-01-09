#include "ObjectManager.h"

#include "Core/ObjectManager.h"

namespace OmegaEngine
{

Object* OEObjectManager::createObject()
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

	Object object(id);
	objects.emplace_back(object);
	return &objects.back();
}

void OEObjectManager::destroyObject(Object& obj)
{
	size_t count = 0;
	for (auto& object : objects)
	{
		if (obj == object)
		{
			break;
		}
		++count;
	}
	// completley remove from the list - costly!
	objects.erase(objects.begin() + count);
	freeIds.push_front(obj.getId());
}

}    // namespace OmegaEngine
