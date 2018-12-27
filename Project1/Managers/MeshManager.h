#pragma once

#include "tiny_gltf.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"

#include "Vulkan/Descriptors.h"

#include <memory>
#include <tuple>
#include <unordered_map>


namespace OmegaEngine
{
	// forard decleartions
	struct GltfVertex;
	struct GltfStaticMeshInfo;
	class ObjectManager;
	class MaterialManager;

	class MeshManager
	{

	public:

		struct Vertex
		{
			OEMaths::vec4f position;
			OEMaths::vec3f normal;
			OEMaths::vec2f uv;
		};

		struct SkinnedVertex : public Vertex
		{
			OEMaths::vec4f joint;
			float weight;
		};

		struct Mesh
		{
			uint32_t indexBase = 0;
			uint32_t indexCount = 0;

			uint32_t MaterialIndex;

			// vulkan info 
			Vulkan::Descriptors descriptors;
		};
		
		struct StaticMesh : public Mesh
		{
			
		};

		struct SkinnedMesh : public Mesh
		{

		};


		MeshManager();
		~MeshManager();

		uint32_t getManagerId();
		void addData(std::vector<GltfVertex>& vertBuf, std::vector<uint32_t>& indBuf, std::vector<GltfStaticMeshInfo>& meshBuf, std::unique_ptr<ObjectManager>& objectManager, SpaceInfo& space);

	private:

		// the buffers containing all the model data for all spaces that are loaded. 
		std::unordered_map<uint32_t, std::vector<Mesh> > staticMeshBuffer;
		std::unordered_map<uint32_t, std::vector<SkinnedMesh> > skinnedMeshBuffer;

		// all vertex and index data is stored in one long continous buffer accessed by offsets for each space. This allows for this data to be used by all spaces
		std::vector<Vertex> vertexBuffer;
		std::vector<uint32_t> indexBuffer;

		std::unique_ptr<MaterialManager> materialManager;
	};

}

