#pragma once
#include "Systems/system.h"
#include "Engine/ModelResourceManager.h"
#include <vector>
#include <string>

// the middle-man system between the omega engine front-end graphic processes and the low-level vulkan stuff

class VulkanEngine;

class GraphicsSystem : public System
{

public:
	GraphicsSystem(VulkanEngine *engine, std::vector<std::string> filenames);
	~GraphicsSystem();

	void Init();
	void Update() override;
	void Destroy() override;
	void Render();

private:

	ModelResourceManager modelManager;

	std::vector<std::string> m_modelFilenames;

	VulkanEngine *p_vkEngine;

};

