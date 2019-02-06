#pragma once

#include <memory>

namespace OmegaEngine
{

	// forward declearations
	class ObjectManager;

	class ManagerBase
	{

	public:

		virtual ~ManagerBase() {}

		// abstract functions
		virtual void update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager) = 0;

		void set_id(const uint32_t id)
		{
			manager_id = id;
		}

	protected:

		uint32_t manager_id = 0;
	};

}