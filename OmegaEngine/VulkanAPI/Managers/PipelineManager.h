#pragma once

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Shader.h"

#include <memory>
#include <set>
#include <unordered_map>

namespace VulkanAPI
{

/**
* @brief Everything needed for the pipeline 
*/
class PStateInfo;

struct PipelineState
{
	VulkanAPI::Shader shader;
	VulkanAPI::PipelineLayout pipelineLayout;
	VulkanAPI::Pipeline pipeline;
	VulkanAPI::DescriptorLayout descriptorLayout;
	VulkanAPI::DescriptorSet descriptorSet;

	// information extracted from shader reflection
	VulkanAPI::BufferReflect bufferLayout;
	VulkanAPI::ImageReflect imageLayout;
};

enum class StateType
{
	Mesh,
	Skybox,
	ShadowMapped,
};

enum class StateAlpha
{
	Opaque,
	Transparent,
	Masked
};

enum class StateTopology
{
	List,
	Strip,
	StripRestart
};

enum class StateFill
{
	Fill,
	WireFrame
};

enum class StateMesh
{
	Static,
	Skinned
};

struct StateId
{
	StateId() = default;
	StateId(const StateType _type)
	    : type(_type)
	{
	}

	StateId(const StateType _type, const StateTopology topo, const StateAlpha _alpha,
	        const StateFill _fill, const StateMesh _mesh)
	    : type(_type)
	    , flags(topo, _alpha, _fill, _mesh)
	{
	}

	StateType type;
	struct StateFlags
	{
		StateFlags() = default;
		StateFlags(const StateTopology topo, const StateAlpha _alpha, const StateFill _fill,
		           const StateMesh _mesh)
		    : topology(topo)
		    , alpha(_alpha)
		    , fill(_fill)
		    , mesh(_mesh)
		{
		}

		StateTopology topology;
		StateAlpha alpha;
		StateFill fill;
		StateMesh mesh;

	} flags;
};

struct StateHash
{
	size_t operator()(StateId const& id) const noexcept
	{
		size_t h1 = std::hash<StateType>{}(id.type);
		size_t h2 = std::hash<StateTopology>{}(id.flags.topology);
		size_t h3 = std::hash<StateAlpha>{}(id.flags.alpha);
		size_t h4 = std::hash<StateFill>{}(id.flags.fill);
		size_t h5 = std::hash<StateMesh>{}(id.flags.mesh);
		return h1 ^ (h2 << 1) ^ (h3 << 1) ^ (h4 << 1) ^ (h5 << 1);
	}
};

struct StateEqual
{
	bool operator()(const StateId& lhs, const StateId& rhs) const
	{
		return lhs.type == rhs.type && lhs.flags.topology == rhs.flags.topology && lhs.flags.alpha == rhs.flags.alpha &&
		       lhs.flags.fill == rhs.flags.fill;
	}
};

class PipelineManager
{
public:
	PipelineManager();
	~PipelineManager();

	void build(const PStateInfo& pInfo);

	static void reflect(uint32_t* data, size_t dataSize, Pipeline& pipeline);

	// makes sense to have the pipeline build info class friends with the manager
	friend class PStateInfo;

private:

	std::unordered_map<StateId, std::unique_ptr<PipelineState>, StateHash, StateEqual> states;
};

}    // namespace OmegaEngine
