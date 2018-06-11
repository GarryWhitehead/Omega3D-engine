#pragma once
#include "Systems/system.h"
#include <vector>
#include <string>
#include "glm.hpp"

class MessageHandler;
class VulkanEngine;
class World;

class GraphicsSystem : public System
{

public:
	GraphicsSystem(World *world, MessageHandler *msg, VulkanEngine *engine);
	~GraphicsSystem();

	void Init();
	void Update() override;
	void Destroy() override;
	void OnNotify(Message& msg) override;

	void Render();
	std::array<glm::mat4, 256> RequestTransformData();

private:

	World *p_world;

	VulkanEngine *p_vkEngine;

};

