#include "ComponentInterface.h"
#include "ObjectInterface/ComponentTypes.h"
#include "ObjectInterface/Object.h"
#include "Managers/ManagerBase.h"
#include "Managers/MeshManager.h"
#include "Managers/TransformManager.h"


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
				manager.addComponentToManager(object->getComponent<MeshComponent>(), *object);
				break;
			}
			else if (object->hasComponent<TransformComponent>())
			{
				auto& manager = getManager<TransformManager>();
				manager.addComponentToManager(object->getComponent<TransformComponent>(), *object);
				break;
			}
		}
	}

	void ComponentInterface::update(double time, double dt, std::unique_ptr<ObjectManager>& objectManager)
	{
		for (auto& manager : managers) 
		{
			manager.second->updateFrame(time, dt, objectManager, this);
		}
	}
}
