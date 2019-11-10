#pragma once

#include "utility/String.h"

#include <unordered_map>
#include <vector>

namespace OmegaEngine
{

class PBuildInfo
{
public:
	enum class ShaderTarget
	{
		Vertex,
		Fragment,
		Compute
	};

	PBuildInfo() = default;

	void addShaderPath(const ShaderTarget target, Util::String path)
	{
		shaderPaths.emplace(target, path);
	}

	void addShaderDefines(Util::String define, const uint8_t value)
	{
	}

	void addUboTargets(Util::String...)
	{
	}

	void setTopology()
	{
	}

	// makes sense to have the pipeline build info class friends with the manager
	friend class VulkanAPI::PipelineManager;

private:
	// shaders
	std::unordered_map<ShaderTarget, Util::String> shaderPaths;
	std::unordered_map<Util::String, uint8_t> shaderDefines;

	// uniform buffers
	std::vector<Util::String> uboTargets;

	// pipeline config
	StateTopology topology;
};

}    // namespace OmegaEngine