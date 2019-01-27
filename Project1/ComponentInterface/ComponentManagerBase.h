#pragma once

namespace OmegaEngine
{

	// forward declearations

	class ComponentManagerBase
	{

	public:

		virtual ~ComponentManagerBase() {}

		// abstract functions
		virtual void update(double time, double dt) = 0;

	protected:

	};

}