#pragma once

#include <memory>
#include "Objects/ObjectManager.h"

namespace OmegaEngine
{

	class ManagerBase
	{

	public:

		ManagerBase() {}
		virtual ~ManagerBase() {}

		// virtual update function -
		virtual void update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager)
		{
		}

		void set_id(const uint32_t id)
		{
			manager_id = id;
		}

	protected:

		uint32_t manager_id = 0;
	};

}