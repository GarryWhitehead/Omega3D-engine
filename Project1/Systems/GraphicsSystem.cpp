#include "GraphicsSystem.h"
#include "utility/message_handler.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VulkanModel.h"
#include "Engine/World.h"
#include "ComponentManagers/TransformComponentManager.h"
#include "ComponentManagers/MeshComponentManager.h"

GraphicsSystem::GraphicsSystem(World *world, MessageHandler *msg, VulkanEngine *engine) :
	System(msg),
	p_world(world),
	p_vkEngine(engine)
{
	Init();
}


GraphicsSystem::~GraphicsSystem()
{
}

void GraphicsSystem::Init()
{
	assert(p_world != nullptr);

	// initialise all the vulkan components required to draw the scene for this world
	p_vkEngine->Init(p_world);

	p_vkEngine->RegisterVulkanModules({ VkModId::VKMOD_SKYBOX_ID, VkModId::VKMOD_WATER_ID, VkModId::VKMOD_TERRAIN_ID, VkModId::VKMOD_MODEL_ID});

	// register this sytem with the message handler
	p_message->AddListener(ListenerID::GRAPHICS_MSG, NotifyResponse());
}

void GraphicsSystem::Update()
{

}

void GraphicsSystem::Render()
{
	// draw the actual scene
	p_vkEngine->Render();
}

std::array<glm::mat4, 256> GraphicsSystem::RequestTransformData()
{
	auto transformManager = p_world->RequestComponentManager<TransformComponentManager>();
	std::array<glm::mat4, 256> transformData = transformManager->DownloadWorldTransformData();

	return transformData;
}

void GraphicsSystem::Destroy()
{

}

void GraphicsSystem::OnNotify(Message& msg)
{

}