#pragma once
#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Common.h"

// forward declerations

namespace OmegaEngine
{
struct RenderConfig;

struct PostProcessInfo
{
	// render callback function
	void (*renderFunction)(void *, void *);
	void *postProcessHandle;

	// data specific to the renderable - mainly drawing information
	void *postProcessData;
};

class PostProcessInterface
{

public:
	PostProcessInterface(vk::Device dev);
	~PostProcessInterface();

	vk::ImageView createPipelines(vk::ImageView &forwardImage, RenderConfig &renderConfig);
	void render(RenderConfig &renderConfig);

private:
	vk::Device device;
	vk::PhysicalDevice gpu;

	std::vector<PostProcessInfo> postProcessPasses;
};

} // namespace OmegaEngine
