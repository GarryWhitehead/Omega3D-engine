#include "RenderInterface.h"
#include "ComponentInterface/ComponentInterface.h"
#include "DataTypes/Object.h"
#include "Managers/TransformManager.h"
#include "Managers/MeshManager.h"
#include "Utility/logger.h"

namespace OmegaEngine
{

	RenderInterface::RenderInterface(VulkanAPI::Device device, const uint32_t win_width, const uint32_t win_height)
	{
		// initiliase the graphical backend - we are solely using Vulkan 
		vk_interface = std::make_unique<VulkanAPI::Interface>(device, win_width, win_height);

		// now initialise all the core graphical modules such as the deferred shader, etc. in conjuction with the render configs
	}


	RenderInterface::~RenderInterface()
	{
	}

	void RenderInterface::add_static_mesh(std::unique_ptr<ComponentInterface>& comp_interface, Object& obj)
	{
		auto &transform_man = comp_interface->getManager<TransformManager>();
		auto &mesh_man = comp_interface->getManager<MeshManager>();

		MeshManager::StaticMesh mesh = mesh_man->getStaticMesh(obj);

		// upload mesh data to GPU
		RenderableMesh r_mesh;


	}

}
