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

	

	void PostProcessInterface::init()
	{
		if (renderConfig.postProcess.useHdr)
		{
			
		}
	}

	void PostProcessInterface::renderToSurface()
	{
		uint32_t imageCount = cmdBufferManager->getPresentImageCount();
		for (uint32_t i = 0; i < imageCount; ++i) 
		{
			auto& cmdBuffer = cmdBufferManager->beginPresentCmdBuffer(swapchain.getRenderpass(), renderConfig.general.backgroundColour, i);
			
		}
	}

	void PostProcessInterface::render(RenderConfig& renderConfig)
	{
		// run through all post processing passes required
		// if there is no post-processing, render straight to the surface
		if (postProcessPasses.empty())
		{
			renderToSurface();
		}

		for (uint32_t i = 0; i < postProcessPasses.size(); ++i)
		{

		}
		
	}
}