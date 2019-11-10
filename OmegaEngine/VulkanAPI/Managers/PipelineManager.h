#pragma once

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Shader.h"
#include "VulkanAPI/Common.h"

#include <memory>
#include <unordered_map>

namespace OmegaEngine
{
class PBuildInfo;
}

namespace VulkanAPI
{
// forward declerations;
class VkContext;



enum class StateAlpha
{
	Opaque,
	Transparent,
	Masked
};




class PipelineManager
{
public:
	PipelineManager();
	~PipelineManager();

	bool build(OmegaEngine::PBuildInfo& pInfo, PStateInfo& state);

private:
	VkContext* context;

	std::unordered_map<StateId, std::unique_ptr<PStateInfo>, StateHash, StateEqual> states;
};

}    // namespace VulkanAPI
