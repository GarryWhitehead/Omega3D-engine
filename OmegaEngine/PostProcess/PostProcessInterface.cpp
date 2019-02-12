#include "PostProcessInterface.h"

namespace OmegaEngine
{

	PostProcessInterface::PostProcessInterface(vk::Device dev) :
		device(dev)
	{
	}


	PostProcessInterface::~PostProcessInterface()
	{
	}

	void PostProcessInterface::render()
	{

	}
}