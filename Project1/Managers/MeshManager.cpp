#include "MeshManager.h"
#include "Utility/logger.h"
#include "Utility/GeneralUtil.h"
#include "Engine/Omega_Global.h"
#include "Objects/ObjectManager.h"
#include "Objects/Object.h"
#include "OEMaths/OEMaths_transform.h"

namespace OmegaEngine
{

	MeshManager::MeshManager()
	{
	}


	MeshManager::~MeshManager()
	{
	}

	void MeshManager::addGltfData(tinygltf::Model& model, tinygltf::Node& node, Object& obj)
	{
		tinygltf::Mesh mesh = model.meshes[node.mesh];
		StaticMesh staticMesh;

		// get all the primitives associated with this mesh
		for (uint32_t i = 0; i < mesh.primitives.size(); ++i) {

			tinygltf::Primitive primitive = mesh.primitives[i];

			// if this primitive has no indices associated with it, no point in continuing
			if (primitive.indices < 0) {
				continue;
			}

			// lets get the vertex data....
			if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
				LOGGER_ERROR("Problem parsing gltf file. Appears to be missing position attribute. Exiting....");
				throw std::runtime_error("Mesh data doesn't contain position attribute.");
			}

			tinygltf::Accessor posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
			tinygltf::BufferView posBufferView = model.bufferViews[posAccessor.bufferView];
			float *posBuffer = (float*)model.buffers[posBufferView.buffer].data[posAccessor.byteOffset + posBufferView.byteOffset];

			// find normal data
			float *normBuffer = nullptr;
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
				tinygltf::Accessor normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
				tinygltf::BufferView normBufferView = model.bufferViews[normAccessor.bufferView];
				normBuffer = (float*)model.buffers[normBufferView.buffer].data[normAccessor.byteOffset + normBufferView.byteOffset];
			}

			// and parse uv data
			float *uvBuffer = nullptr;
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
				tinygltf::Accessor uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
				tinygltf::BufferView uvBufferView = model.bufferViews[uvAccessor.bufferView];
				uvBuffer = (float*)model.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset];
			}

			// check whether this model has skinning data - joints first
			uint16_t *jointBuffer = nullptr;
			if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
				tinygltf::Accessor jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
				tinygltf::BufferView jointBufferView = model.bufferViews[jointAccessor.bufferView];
				jointBuffer = (uint16_t*)model.buffers[jointBufferView.buffer].data[jointAccessor.byteOffset + jointBufferView.byteOffset];
			}

			// and then weights. It must contain both to for the data to be used for animations
			float *weightBuffer = nullptr;
			if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
				tinygltf::Accessor weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
				tinygltf::BufferView weightBufferView = model.bufferViews[weightAccessor.bufferView];
				weightBuffer = (float*)model.buffers[weightBufferView.buffer].data[weightAccessor.byteOffset + weightBufferView.byteOffset];
			}

			// get the min and max values for this primitive
			OEMaths::vec3f primMin{ posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2] };
			OEMaths::vec3f primMax{ posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2] };

			// now convert the data to a form that we can use with Vulkan
			for (uint32_t j = 0; j < posAccessor.count; ++j) {
				Vertex vertex;
				vertex.position = OEMaths::vec3_to_vec4(OEMaths::convert_vec3(&posBuffer[j * 3]), 1.0f);

				if (normBuffer) {
					vertex.normal = OEMaths::normalise_vec3(OEMaths::convert_vec3(&normBuffer[j * 3]));
				}


				if (uvBuffer) {
					vertex.uv = OEMaths::convert_vec2(&uvBuffer[j * 2]);
				}

				// if we have skin data, also convert this to a palatable form
				if (weightBuffer && jointBuffer) {
					vertex.joint = OEMaths::convert_vec4(&jointBuffer[j * 4]);
					vertex.weight = OEMaths::convert_vec4(&weightBuffer[j * 4]);
				}
				staticMesh.vertexBuffer.push_back(vertex);
			}

			// Now obtain the indicies data from the gltf file
			tinygltf::Accessor indAccessor = model.accessors[primitive.indices];
			tinygltf::BufferView indBufferView = model.bufferViews[indAccessor.bufferView];
			tinygltf::Buffer indBuffer = model.buffers[indBufferView.buffer];

			uint32_t indexCount = indAccessor.count;
			uint32_t indexOffset = staticMesh.indexBuffer.size();

			// the indicies can be stored in various formats - this can be determined from the component type in the accessor
			switch (indAccessor.componentType) {
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
				parseIndices<uint32_t>(indAccessor, indBufferView, indBuffer, staticMesh.indexBuffer, indexOffset);
				break;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
				parseIndices<uint16_t>(indAccessor, indBufferView, indBuffer, staticMesh.indexBuffer, indexOffset);
				break;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
				parseIndices<uint8_t>(indAccessor, indBufferView, indBuffer, staticMesh.indexBuffer, indexOffset);
				break;
			}
			default:
				throw std::runtime_error("Unable to parse indices data. Unsupported accessor component type.");
			}

			PrimitiveMesh prim(indexOffset, indexCount, (uint32_t)primitive.material, primMin, primMax);
			staticMesh.primitives.push_back(prim);
		}

		meshBuffer.push_back(staticMesh);
		
		// add mesh component to current object
		obj.add_manager<MeshManager>(meshBuffer.size() - 1);
	}

	

	
	
}
