#include "ComponentInterface.h"
#include "Managers/AnimationManager.h"
#include "Managers/ManagerBase.h"
#include "Managers/MeshManager.h"
#include "Managers/TransformManager.h"
#include "Managers/MaterialManager.h"
#include "ObjectInterface/ComponentTypes.h"
#include "ObjectInterface/Object.h"

namespace OmegaEngine
{

ComponentInterface::ComponentInterface()
{
}

ComponentInterface::~ComponentInterface()
{
}

void ComponentInterface::addObjectToUpdateQueue(Object *object)
{
	assert(object != nullptr);
	objectUpdateQueue.emplace_back(object);
}

void ComponentInterface::updateManagersRecursively(Object *object)
{
	if (object->hasComponent<MeshComponent>())
	{
		auto &manager = getManager<MeshManager>();
		manager.addComponentToManager(&object->getComponent<MeshComponent>());
	}
	if (object->hasComponent<TransformComponent>())
	{
		auto &manager = getManager<TransformManager>();
		manager.addComponentToManager(&object->getComponent<TransformComponent>());
	}
	if (object->hasComponent<MaterialComponent>())
	{
		auto &materialManager = getManager<MaterialManager>();
		materialManager.addComponentToManager(&object->getComponent<MaterialComponent>());

		// link material with mesh
		auto &meshManager = getManager<MeshManager>();
		meshManager.linkMaterialWithMesh(&object->getComponent<MeshComponent>(),
										 &object->getComponent<MaterialComponent>());
	}
	if (object->hasComponent<SkinnedComponent>())
	{
	}
	if (object->hasComponent<AnimationComponent>())
	{
		auto &manager = getManager<AnimationManager>();
		manager.addComponentToManager(&object->getComponent<AnimationComponent>(), *object);
	}
	if (object->hasComponent<SkeletonComponent>())
	{
		auto &manager = getManager<TransformManager>();
		manager.addComponentToManager(&object->getComponent<SkeletonComponent>(), object);
	}

	if (object->hasChildren())
	{
		for (auto &child : object->getChildren())
		{
			updateManagersRecursively(&child);
		}
	}
}

void ComponentInterface::updateManagersFromQueue()
{
	if (objectUpdateQueue.empty())
	{
		return;
	}

	for (auto object : objectUpdateQueue)
	{
		updateManagersRecursively(object);
	}

	// make sure you clear here!!
	objectUpdateQueue.clear();
}

void ComponentInterface::update(double time, double dt,
                                std::unique_ptr<ObjectManager> &objectManager)
{
	// first, check whether any new components have been added. If so, add them to the managers
	this->updateManagersFromQueue();

	for (auto &manager : managers)
	{
		manager.second->updateFrame(time, dt, objectManager, this);
	}
}
} // namespace OmegaEngine
