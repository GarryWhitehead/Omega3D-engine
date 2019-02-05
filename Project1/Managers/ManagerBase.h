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

	protected:

	};

}