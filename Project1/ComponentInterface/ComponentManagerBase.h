#pragma once

namespace OmegaEngine
{

	// forward declearations

	class ComponentManagerBase
	{

	public:

		virtual ~ComponentManagerBase() {}

		// abstract functions
		virtual void update() = 0;

	protected:

	};

}