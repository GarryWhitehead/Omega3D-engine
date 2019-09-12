#pragma once

#include "utility/String.h"

#include <unordered_map>
#include <vector>

namespace OmegaEngine
{

class PStateInfo
{
public:
	enum class ShaderTarget
	{
		Vertex,
		Fragment,
		Compute
	};

	PStateInfo() = default;

	void addShaderPath(const ShaderTarget target, Util::String path)
	{
		shaderPaths.emplace(target, path);
	}

	void addUboTargets(Util::String...)
	{
	}

	void setTopology()
	{
	
	}

private:
	// shaders
	std::unordered_map<ShaderTarget, Util::String> shaderPaths;

	// uniform buffers
	std::vector<Util::String> uboTargets;

	// pipeline config
	StateTopology topology;
};

}    // namespace OmegaEngine