#include "MeshManager.h"
#include "Utility/logger.h"
#include "Utility/GeneralUtil.h"
#include "Engine/Omega_Global.h"
#include "Objects/ObjectManager.h"
#include "Objects/Object.h"
#include "Vulkan/Vulkan_Global.h"
#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine
{

	MeshManager::MeshManager()
	{
		VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;
		// non-skinned
		vertex_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_STATIC, sizeof(Vertex) * static_cast<uint32_t>(VertexBlockSize));
		index_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_STATIC, sizeof(uint32_t) * static_cast<uint32_t>(IndexBlockSize));
		// skinned
		skinned_vertex_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_STATIC, sizeof(SkinnedVertex) * static_cast<uint32_t>(VertexBlockSize));
		skinned_index_buffer = mem_alloc.allocate(VulkanAPI::MemoryUsage::VK_BUFFER_STATIC, sizeof(uint32_t) * static_cast<uint32_t>(IndexBlockSize));
	}


	MeshManager::~MeshManager()
	{
	}

	void MeshManager::addGltfData(tinygltf::Model& model, tinygltf::Node& node, Object* obj)
	{
		const tinygltf::Mesh& mesh = model.meshes[node.mesh];
		
		StaticMesh staticMesh;

		// get all the primitives associated with this mesh
		for (uint32_t i = 0; i < mesh.primitives.size(); ++i) {

			const tinygltf::Primitive& primitive = mesh.primitives[i];

			// if this primitive has no indices associated with it, no point in continuing
			if (primitive.indices < 0) {
				continue;
			}

			// lets get the vertex data....
			if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
				LOGGER_ERROR("Problem parsing gltf file. Appears to be missing position attribute. Exiting....");
			}

			const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
			const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
			const float *posBuffer = reinterpret_cast<const float*>(&(model.buffers[posBufferView.buffer].data[posAccessor.byteOffset + posBufferView.byteOffset]));

			// find normal data
			const float *normBuffer = nullptr;
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
				const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
				const tinygltf::BufferView& normBufferView = model.bufferViews[normAccessor.bufferView];
				normBuffer = reinterpret_cast<const float*>(&(model.buffers[normBufferView.buffer].data[normAccessor.byteOffset + normBufferView.byteOffset]));
			}

			// and parse uv data - there can be two tex coord buffers, we need to check for both
			const float *uvBuffer0 = nullptr;
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
				const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
				const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
				uvBuffer0 = reinterpret_cast<const float*>(&(model.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
			}

			const float *uvBuffer1 = nullptr;
			if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
				const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
				const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
				uvBuffer1 = reinterpret_cast<const float*>(&(model.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
			}

			// check whether this model has skinning data - joints first
			const uint16_t *jointBuffer = nullptr;
			if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
				const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
				const tinygltf::BufferView& jointBufferView = model.bufferViews[jointAccessor.bufferView];
				jointBuffer = reinterpret_cast<const uint16_t*>(&(model.buffers[jointBufferView.buffer].data[jointAccessor.byteOffset + jointBufferView.byteOffset]));
			}

			// and then weights. It must contain both to for the data to be used for animations
			const float *weightBuffer = nullptr;
			if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
				const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
				const tinygltf::BufferView& weightBufferView = model.bufferViews[weightAccessor.bufferView];
				weightBuffer = reinterpret_cast<const float*>(&(model.buffers[weightBufferView.buffer].data[weightAccessor.byteOffset + weightBufferView.byteOffset]));
			}

			// get the min and max values for this primitive
			OEMaths::vec3f primMin{ 0.0f, 0.0f, 0.0f }; // { posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2] };
			OEMaths::vec3f primMax{ 0.0f, 0.0f, 0.0f };  //{ posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2] };

			// now convert the data to a form that we can use with Vulkan - skinned and non-skinned meshes treated separately
			MeshType mesh_type;
			if (weightBuffer && jointBuffer) {

				for (uint32_t j = 0; j < posAccessor.count; ++j) {
					SkinnedVertex vertex; 
					vertex.position = OEMaths::vec3_to_vec4(OEMaths::convert_vec3<float>(posBuffer), 1.0f);
					posBuffer += 3;

					if (normBuffer) {
						vertex.normal = OEMaths::normalise_vec3(OEMaths::convert_vec3<float>(normBuffer));
						normBuffer += 3;
					}
					if (uvBuffer0) {
						vertex.uv0 = OEMaths::convert_vec2<float>(uvBuffer0);
						uvBuffer0 += 2;
					}
					if (uvBuffer1) {
						vertex.uv1 = OEMaths::convert_vec2<float>(uvBuffer1);
						uvBuffer1 += 2;
					}

					// if we have skin data, also convert this to a palatable form
					vertex.joint = OEMaths::convert_vec4<float>(reinterpret_cast<const float*>(jointBuffer));
					vertex.weight = OEMaths::convert_vec4<float>(weightBuffer);
					jointBuffer += 4;
					weightBuffer += 4;

					mesh_type = MeshType::Skinned;
					staticMesh.skinnedVertexBuffer.push_back(vertex);
				}
			}
			else {
				for (uint32_t j = 0; j < posAccessor.count; ++j) {
					Vertex vertex; 
					vertex.position = OEMaths::vec3_to_vec4(OEMaths::convert_vec3<float>(posBuffer), 1.0f);
					posBuffer += 3;

					if (normBuffer) {
						vertex.normal = OEMaths::normalise_vec3(OEMaths::convert_vec3<float>(normBuffer));
						normBuffer += 3;
					}
					if (uvBuffer0) {
						vertex.uv0 = OEMaths::convert_vec2<float>(uvBuffer0);
						uvBuffer0 += 2;
					}
					if (uvBuffer1) {
						vertex.uv1 = OEMaths::convert_vec2<float>(uvBuffer1);
						uvBuffer1 += 2;
					}
					mesh_type = MeshType::Static;
					staticMesh.vertexBuffer.push_back(vertex);
				}
			}

			// Now obtain the indicies data from the gltf file
			const tinygltf::Accessor& indAccessor = model.accessors[primitive.indices];
			const tinygltf::BufferView& indBufferView = model.bufferViews[indAccessor.bufferView];
			const tinygltf::Buffer& indBuffer = model.buffers[indBufferView.buffer];

			uint32_t indexCount = static_cast<uint32_t>(indAccessor.count);
			uint32_t indexOffset = static_cast<uint32_t>(staticMesh.indexBuffer.size());

			// the indicies can be stored in various formats - this can be determined from the component type in the accessor
			switch (indAccessor.componentType) {
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
				parseIndices<uint32_t>(indAccessor, indBufferView, indBuffer, staticMesh.indexBuffer, indexOffset);
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
				parseIndices<uint16_t>(indAccessor, indBufferView, indBuffer, staticMesh.indexBuffer, indexOffset);
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
				parseIndices<uint8_t>(indAccessor, indBufferView, indBuffer, staticMesh.indexBuffer, indexOffset);
				break;
			}
			default:
				throw std::runtime_error("Unable to parse indices data. Unsupported accessor component type.");
			}

			staticMesh.type = mesh_type;
			PrimitiveMesh prim(mesh_type, indexOffset, indexCount, (uint32_t)primitive.material, primMin, primMax);
			staticMesh.primitives.push_back(prim);
		}

		meshBuffer.push_back(staticMesh);
				
		// add mesh component to current object
		obj->add_manager<MeshManager>(static_cast<uint32_t>(meshBuffer.size() - 1));
	}

	void MeshManager::update_frame(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager, ComponentInterface* component_interface)
	{
		if (isDirty) {
			
			VulkanAPI::MemoryAllocator &mem_alloc = VulkanAPI::Global::Managers::mem_allocator;	
			uint32_t skinned_vertex_offset = 0;
			uint32_t skinned_index_offset = 0;
			uint32_t vertex_offset = 0;
			uint32_t index_offset = 0;

			// non-skinned meshes
			for (auto& mesh : meshBuffer) {

				if (mesh.type == MeshManager::MeshType::Static) {
					mem_alloc.mapDataToSegment(vertex_buffer, mesh.vertexBuffer.data(), static_cast<uint32_t>(mesh.vertexBuffer.size()) * sizeof(Vertex), vertex_offset);
					mem_alloc.mapDataToSegment(index_buffer, mesh.indexBuffer.data(), static_cast<uint32_t>(mesh.indexBuffer.size()) * sizeof(uint32_t), index_offset);

					mesh.vertex_buffer_offset = vertex_offset;
					mesh.index_buffer_offset = index_offset;

					vertex_offset += mesh.vertexBuffer.size() * sizeof(Vertex);	// in bytes!!!
					index_offset += mesh.indexBuffer.size() * sizeof(uint32_t);
				}
				// skinned meshes
				else if (mesh.type == MeshManager::MeshType::Skinned) {
					mem_alloc.mapDataToSegment(skinned_vertex_buffer, mesh.skinnedVertexBuffer.data(), static_cast<uint32_t>(mesh.skinnedVertexBuffer.size()) * sizeof(SkinnedVertex), skinned_vertex_offset);
					mem_alloc.mapDataToSegment(skinned_index_buffer, mesh.indexBuffer.data(), static_cast<uint32_t>(mesh.indexBuffer.size()) * sizeof(uint32_t), skinned_index_offset);

					mesh.vertex_buffer_offset = skinned_vertex_offset;
					mesh.index_buffer_offset = skinned_index_offset;

					skinned_vertex_offset += mesh.skinnedVertexBuffer.size() * sizeof(SkinnedVertex);	// in bytes!!!
					skinned_index_offset += mesh.indexBuffer.size() * sizeof(uint32_t);
				}
			}
		}

		isDirty = false;
	}
}
