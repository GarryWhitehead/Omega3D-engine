#pragma once
#include "Systems/system.h"
#include <vector>
#include <string>
#include "glm.hpp"

// the middle-man system between the omega engine front-end graphic processes and the low-level vulkan stuff

class VulkanEngine;
class World;

class GraphicsSystem : public System
{

public:
	GraphicsSystem(World *world, VulkanEngine *engine);
	~GraphicsSystem();

	void Init();
	void Update() override;
	void Destroy() override;
	void Render();
	std::array<glm::mat4, 256> RequestTransformData();
	void InitVulkanModel();

private:

	World *p_world;

	VulkanEngine *p_vkEngine;

};

