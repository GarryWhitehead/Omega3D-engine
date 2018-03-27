#include "GraphicsSystem.h"
#include "VulkanCore/VulkanEngine.h"
#include "ComponentManagers/TransformComponentManager.h"
#include "ComponentManagers/MeshComponentManager.h"

GraphicsSystem::GraphicsSystem(VulkanEngine *engine, std::vector<std::string> filenames) :
	p_vkEngine(engine),
	m_modelFilenames(filenames)
{
}


GraphicsSystem::~GraphicsSystem()
{
}

void GraphicsSystem::Init()
{
	// initialise all the vulkan components required to draw the scene for this world
	p_vkEngine->Init();

	p_vkEngine->RegisterVulkanModules({ VkModId::VKMOD_SHADOW_ID, VkModId::VKMOD_TERRAIN_ID, VkModId::VKMOD_MODEL_ID });

	p_vkEngine->RegisterGraphicsSystem(this);

	// create model resource manager responsible for importing and controlling models within this space
	modelManager.RegisterVulkanEngine(p_vkEngine);
	modelManager.PrepareModelResources(m_modelFilenames);
}

void GraphicsSystem::Update()
{

}

void GraphicsSystem::Render()
{
	// prepare the updated data for rendering
	auto transformManager = GetRegisteredManager<TransformComponentManager>(ComponentManagerId::CM_TRANSFORM_ID);
	auto meshManager = GetRegisteredManager<MeshComponentManager>(ComponentManagerId::CM_MESH_ID);

	// upload any updated data required for rendering - transform and mesh data - mesh data is stored within the model manager as it is required
	// by the vulkan model module
  	modelManager.DownloadTransformData(transformManager);
	modelManager.DownloadMeshData(meshManager);

	// draw the actual scene
	p_vkEngine->Render();
}

void GraphicsSystem::Destroy()
{

}