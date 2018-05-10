#include "GraphicsSystem.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VulkanModel.h"
#include "Engine/World.h"
#include "ComponentManagers/TransformComponentManager.h"
#include "ComponentManagers/MeshComponentManager.h"

GraphicsSystem::GraphicsSystem(World *world, VulkanEngine *engine) :
	p_world(world),
	p_vkEngine(engine)
{
}


GraphicsSystem::~GraphicsSystem()
{
}

void GraphicsSystem::Init()
{
	// initialise all the vulkan components required to draw the scene for this world
	assert(p_world != nullptr);
	p_vkEngine->Init(p_world);

	p_vkEngine->RegisterGraphicsSystem(this);
	
	p_vkEngine->RegisterVulkanModules({ VkModId::VKMOD_SHADOW_ID, VkModId::VKMOD_PBR_ID, VkModId::VKMOD_IBL_ID, VkModId::VKMOD_DEFERRED_ID, VkModId::VKMOD_SKYBOX_ID, VkModId::VKMOD_TERRAIN_ID, VkModId::VKMOD_MODEL_ID});
}

void GraphicsSystem::InitVulkanModel()
{
	p_vkEngine->VkModule<VulkanModel>(VkModId::VKMOD_MODEL_ID)->Init();
}

void GraphicsSystem::Update()
{

}

void GraphicsSystem::Render()
{
	// draw the actual scene
	p_vkEngine->Render();
}

void GraphicsSystem::RequestTransformData(glm::mat4 transformData[256])
{
	auto transformManager = p_world->RequestComponentManager<TransformComponentManager>(ComponentManagerId::CM_TRANSFORM_ID);
	transformManager->DownloadWorldTransformData(transformData);
}

void GraphicsSystem::Destroy()
{

}