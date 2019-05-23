#pragma once

#include "Objects/ObjectManager.h"

#include <memory>

namespace OmegaEngine
{
	// forward declerations
	class ComponentInterface;

	class ManagerBase
	{

	public:

		ManagerBase() {}
		virtual ~ManagerBase() {}

		// virtual update function -
		virtual void updateFrame(double time, double dt,
			std::unique_ptr<ObjectManager>& obj_manager,
			ComponentInterface* component_manager) = 0;

		void setId(const uint32_t id)
		{
			manager_id = id;
		}

	protected:

		uint32_t manager_id = 0;
	};

}