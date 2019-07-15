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

// hasher for state id
namespace std
{
template <>
struct hash<OmegaEngine::ProgramStateManager::StateId>
{
	size_t operator()(OmegaEngine::ProgramStateManager::StateId const &id) const
	{
		    return ((hash<OmegaEngine::ProgramStateManager::StateType>()(id.type) ^ (hash<OmegaEngine::ProgramStateManager::StateFlags>()(id.flags) << 1) >> 1);
	}
};

} // namespace std

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

	bool operator==(const StateId &other) const
	{
		return type == other.type && flags.topology == other.flags.topology &&
		       flags.alpha == other.flags.alpha && flags.fill == other.flags.fill;
	}
};

class ProgramStateManager
{
public:
	ProgramStateManager();
	~ProgramStateManager();

	ProgramState *createState(
	    std::unique_ptr<VulkanAPI::Interface> &vkInterface, std::unique_ptr<RendererBase> &renderer,
	    const StateType type, const StateTopology topology, const StateMesh meshType,
	    const StateAlpha alpha);

	void queueState(StateId &id)
	{
		stateQueue.insert(id);
	}

private:
	std::set<StateId> stateQueue;

	std::unordered_map<StateId, std::unique_ptr<ProgramState>> states;
};

} // namespace OmegaEngine
