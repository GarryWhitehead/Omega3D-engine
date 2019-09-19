#include "PostProcessInterface.h"
#include "Rendering/RenderConfig.h"
#include "VulkanAPI/Image.h"

namespace OmegaEngine
{

PostProcessInterface::PostProcessInterface(vk::Device dev)
    : device(dev)
{
}

PostProcessInterface::~PostProcessInterface()
{
}

vk::ImageView PostProcessInterface::createPipelines(vk::ImageView& forwardImage, RenderConfig& renderConfig)
{
	vk::ImageView finalImage = forwardImage;

	if (renderConfig.postProcess.useHdr)
	{
	}

	return finalImage;
}

void PostProcessInterface::render(RenderConfig& renderConfig)
{
	// run through all post processing passes required
	for (auto& pass : postProcessPasses)
	{
		pass.renderFunction(pass.postProcessHandle, pass.postProcessData);
	}
}
}    // namespace OmegaEngine