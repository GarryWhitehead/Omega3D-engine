#pragma once

#include "VulkanAPI/Descriptors.h"
#include "VulkanAPI/Pipeline.h"
#include "VulkanAPI/Shader.h"

#include <memory>
#include <set>
#include <unordered_map>

namespace VulkanAPI
{
class Interface;
}

namespace OmegaEngine
{

class RendererBase;

struct ProgramState
{
	VulkanAPI::Shader shader;
	VulkanAPI::PipelineLayout pipelineLayout;
	VulkanAPI::Pipeline pipeline;
	VulkanAPI::DescriptorLayout descriptorLayout;
	VulkanAPI::DescriptorSet descriptorSet;

	// information extracted from shader reflection
	VulkanAPI::BufferReflection bufferLayout;
	VulkanAPI::ImageReflection imageLayout;
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
	Strip
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

	bool operator==(const StateId& other) const
	{
		return type == other.type && flags.topology == other.flags.topology &&
		       flags.alpha == other.flags.alpha && flags.fill == other.flags.fill;
	}
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

class ProgramStateManager
{
public:
	ProgramStateManager();
	~ProgramStateManager();

	ProgramState* createState(std::unique_ptr<VulkanAPI::Interface>& vkInterface,
	                          std::unique_ptr<RendererBase>& renderer, const StateType type,
	                          const StateTopology topology, const StateMesh meshType,
	                          const StateAlpha alpha);

	void queueState(StateId& id)
	{
		stateQueue.insert(id);
	}

private:
	std::set<StateId> stateQueue;

	std::unordered_map<StateId, std::unique_ptr<ProgramState>, StateHash> states;
};

}    // namespace OmegaEngine
