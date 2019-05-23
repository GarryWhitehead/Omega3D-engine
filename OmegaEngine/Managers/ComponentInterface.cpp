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

	void ComponentInterface::update_managers(double time, double dt, std::unique_ptr<ObjectManager>& objectManager)
	{
		for (auto& manager : managers) 
		{
			manager.second->updateFrame(time, dt, objectManager, this);
		}
	}
}
