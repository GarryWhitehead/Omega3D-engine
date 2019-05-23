#include "PostProcessInterface.h"
#include "Rendering/RenderConfig.h"

namespace OmegaEngine
{

	PostProcessInterface::PostProcessInterface(vk::Device dev) :
		device(dev)
	{
	}


	PostProcessInterface::~PostProcessInterface()
	{
	}

	

	void PostProcessInterface::init(RenderConfig& renderConfig)
	{
		if (renderConfig.postProcess.useHdr)
		{
			
		}
	}

	void PostProcessInterface::render(RenderConfig& renderConfig)
	{
		// run through all post processing passes required
		for (auto& pass : postProcessPasses)
		{
			pass.renderFunction(pass.postProcessHandle, pass.postProcessData);
		}
		
	}
}