#include "MeshManager.h"
#include "Utility/logger.h"
#include "Utility/GeneralUtil.h"
#include "Engine/Omega_Global.h"
#include "Engine/GltfParser.h"
#include "Managers/ObjectManager.h"
#include "DataTypes/Space.h"

namespace OmegaEngine
{

	MeshManager::MeshManager()
	{
	}


	MeshManager::~MeshManager()
	{
	}

	void MeshManager::addData(std::vector<GltfVertex>& vertBuf, std::vector<uint32_t>& indBuf, std::vector<GltfStaticMeshInfo>& meshBuf, std::unique_ptr<ObjectManager>& objectManager, SpaceInfo& space)
	{
		if (vertBuf.empty() || indBuf.empty()) {
			LOGGER_INFO("Unable to add mesh data to manager as no vertex or indicies data is available for space id %i.", space.spaceId);
			return;
		}

		assert(objectManager != nullptr);

		// append the vertex data to any exsisting data
		space.vertexOffset = vertexBuffer.size();
		vertexBuffer.insert(vertexBuffer.end(), vertBuf.begin(), vertBuf.end());

		// same with the indicies
		space.indexOffset = indexBuffer.size();
		indexBuffer.insert(indexBuffer.end(), indBuf.begin(), indBuf.end());

		// now generate the mesh data 
		for (auto& gltf_mesh : meshBuf) {
			
			Mesh mesh;


		}
	}

	uint32_t MeshManager::getManagerId()
	{
		return Util::event_type_id<MeshManager>();
	}
}
