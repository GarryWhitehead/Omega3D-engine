#pragma once

namespace OmegaEngine
{

	// forward declearations

	class ManagerBase
	{

	public:

		virtual ~ManagerBase() {}

		// abstract functions
		virtual void update_frame(double time, double dt) = 0;

	protected:

	};

}