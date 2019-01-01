#include "RenderInterface.h"
#include "ComponentInterface/ComponentInterface.h"
#include "DataTypes/Object.h"
#include "Managers/TransformManager.h"
#include "Managers/MeshManager.h"
#include "Utility/logger.h"

namespace OmegaEngine
{

	RenderInterface::RenderInterface()
	{
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
