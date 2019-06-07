#include "ComponentInterface.h"
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
			auto& componentList = object->getComponentList();

			for (auto& component : componentList)
			{
				auto& manager = getManager(component.managerId);
				manager->
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
