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

void ProgramStateManager::createStates(std::unique_ptr<VulkanAPI::Interface> &vkInterface,
                                       std::unique_ptr<RendererBase>& renderer)
{
	for (auto state : stateQueue)
	{
		auto &newState = std::make_unique<ProgramState>();

		switch (state.type)
		{
		case StateType::MeshStatic:
		{
			RenderableMesh::createMeshPipeline(
			    vkInterface, renderer, MeshManager::MeshType::Static, newState, state.flags);
			states.emplace(state, std::move(state));
			break;
		}
		case StateType::MeshSkinned:
		{
			RenderableMesh::createMeshPipeline(
			    vkInterface, renderer, MeshManager::MeshType::Skinned, newState, state.flags);
			states.emplace(state, std::move(state));
			break;
		}
		case StateType::ShadowMapped:
		{
			RenderableShadow::createShadowPipeline(vkInterface->getDevice(), renderer,
			                                       vkInterface->getBufferManager(), state);
			renderStates[(int)RenderTypes::ShadowMapped] = std::move(state);
			break;
		}
		case StateType::Skybox:
		{
			RenderableSkybox::createSkyboxPipeline(vkInterface->getDevice(), renderer,
			                                       vkInterface->getBufferManager(),
			                                       vkInterface->gettextureManager(), state);
			renderStates[(int)RenderTypes::Skybox] = std::move(state);
			break;
		}
		default:
			LOGGER_INFO("Unsupported render type found whilst initilaising shaders.");
		}
	}

	stateQueue.clear();
}

} // namespace OmegaEngine