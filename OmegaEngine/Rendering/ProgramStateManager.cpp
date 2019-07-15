#include "ProgramStateManager.h"
#include "Rendering/RenderableTypes/Mesh.h"
#include "Rendering/RenderableTypes/Skybox.h"
#include "Rendering/RenderableTypes/Shadow.h"
#include "VulkanAPI/Interface.h"

namespace OmegaEngine
{

ProgramStateManager::ProgramStateManager()
{
}

ProgramStateManager::~ProgramStateManager()
{
}
ProgramState* ProgramStateManager::createState(std::unique_ptr<VulkanAPI::Interface> &vkInterface,
                                       std::unique_ptr<RendererBase>& renderer, const StateType type, const StateTopology topology, const StateMesh meshType, const StateAlpha alpha)
{
	// define the state
	StateId id{ type, topology, alpha, StateFill::Fill, meshType };

	// first check whether this state already exists
	if (states.find(id) != states.end())
	{
		return states[id].get();
	}
	
	auto &newState = std::make_unique<ProgramState>();

	switch (type)
	{
	case StateType::MeshStatic:
	{
		RenderableMesh::createMeshPipeline(
			vkInterface, renderer, MeshType::Static, newState, id.flags);
		break;
	}
	case StateType::MeshSkinned:
	{
		RenderableMesh::createMeshPipeline(
			vkInterface, renderer, MeshType::Skinned, newState, id.flags);
		break;
	}
	case StateType::ShadowMapped:
	{
		RenderableShadow::createShadowPipeline(vkInterface->getDevice(), renderer,
			                                    vkInterface->getBufferManager(), id.flags);
		break;
	}
	case StateType::Skybox:
	{
		RenderableSkybox::createSkyboxPipeline(vkInterface->getDevice(), renderer,
			                                    vkInterface->getBufferManager(),
			                                    vkInterface->gettextureManager(), id.flags);
		break;
	}
	default:
		LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
	}

	states.emplace(id, std::move(newState));

	return states[id].get();
}

} // namespace OmegaEngine