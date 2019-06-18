#include "ComponentInterface.h"
#include "ObjectInterface/ComponentTypes.h"
#include "ObjectInterface/Object.h"
#include "Managers/ManagerBase.h"
#include "Managers/MeshManager.h"
#include "Managers/TransformManager.h"
#include "Managers/AnimationManager.h"

namespace OmegaEngine
{

	ComponentInterface::ComponentInterface()
	{
	}


	ComponentInterface::~ComponentInterface()
	{
	}

	void ComponentInterface::addObjectToUpdateQueue(Object* object)
	{
		assert(object != nullptr);
		objectUpdateQueue.emplace_back(object);
	}

	void ComponentInterface::updateManagersFromQueue()
	{
		if (objectUpdateQueue.empty())
		{
			return;
		}

		for (auto object : objectUpdateQueue)
		{
			if (object->hasComponent<MeshComponent>())
			{
				auto& manager = getManager<MeshManager>();
				manager.addComponentToManager(&object->getComponent<MeshComponent>());
			}
			if (object->hasComponent<TransformComponent>())
			{
				auto& manager = getManager<TransformManager>();
				manager.addComponentToManager(&object->getComponent<TransformComponent>());
			}
			if (object->hasComponent<MaterialComponent>())
			{

			}
			if (object->hasComponent<SkinnedComponent>())
			{
				auto& manager = getManager<TransformManager>();
				manager.addComponentToManager(&object->getComponent<SkinnedComponent>(), *object);
			}
			if (object->hasComponent<AnimationComponent>())
			{
				auto& manager = getManager<AnimationManager>();
				manager.addComponentToManager(&object->getComponent<AnimationComponent>(), *object);
			}
		}
	}

	void ComponentInterface::update(double time, double dt, std::unique_ptr<ObjectManager>& objectManager)
	{
		// first, check whether any new components have been added. If so, add them to the managers
		this->updateManagersFromQueue();

		for (auto& manager : managers) 
		{
			manager.second->updateFrame(time, dt, objectManager, this);
		}
	}
}
