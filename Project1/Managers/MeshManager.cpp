#include "MeshManager.h"
#include "Utility/logger.h"

namespace OmegaEngine
{

	MeshManager::MeshManager()
	{
	}


	MeshManager::~MeshManager()
	{
	}

	void MeshManager::parseGltfFile(uint32_t spaceId, tinygltf::Model& model)
	{
		// we are going to parse the node recursively to get all the info required for the space

	}

	void MeshManager::parseNode(tinygltf::Node node, tinygltf::Model& model)
	{
		// if the node has mesh data...
		if (node.mesh > -1) {
			tinygltf::Mesh mesh = model.meshes[node.mesh];

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
				float *jointBuffer = nullptr;
				if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
					tinygltf::Accessor jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
					tinygltf::BufferView jointBufferView = model.bufferViews[jointAccessor.bufferView];
					jointBuffer = (float*)model.buffers[jointBufferView.buffer].data[jointAccessor.byteOffset + jointBufferView.byteOffset];
				}

				// and then weights. It must contain both to for the data to be used for animations
				float *weightBuffer = nullptr;
				if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
					tinygltf::Accessor weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
					tinygltf::BufferView weightBufferView = model.bufferViews[weightAccessor.bufferView];
					weightBuffer = (float*)model.buffers[weightBufferView.buffer].data[weightAccessor.byteOffset + weightBufferView.byteOffset];
				}
			}
		}
	}
}
