#include "ModelMesh.h"
#include "Utility/Logger.h"

namespace OmegaEngine
{

	ModelMesh::ModelMesh()
	{
	}


	ModelMesh::~ModelMesh()
	{
	}

	void ModelMesh::extractMeshData(tinygltf::Model& model, tinygltf::Node& node)
	{
		const tinygltf::Mesh& mesh = model.meshes[node.mesh];

		uint32_t localVertexOffset = 0;
		uint32_t localIndexOffset = 0;

		// get all the primitives associated with this mesh
		for (uint32_t i = 0; i < mesh.primitives.size(); ++i)
		{
			uint32_t vertexStart = static_cast<uint32_t>(vertices.size());
			const tinygltf::Primitive& primitive = mesh.primitives[i];

			// if this primitive has no indices associated with it, no point in continuing
			if (primitive.indices < 0)
			{
				continue;
			}

			// lets get the vertex data....
			if (primitive.attributes.find("POSITION") == primitive.attributes.end())
			{
				LOGGER_ERROR("Problem parsing gltf file. Appears to be missing position attribute. Exiting....");
			}

			const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
			const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
			const float *posBuffer = reinterpret_cast<const float*>(&(model.buffers[posBufferView.buffer].data[posAccessor.byteOffset + posBufferView.byteOffset]));

			// find normal data
			const float *normBuffer = nullptr;
			if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
			{
				const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
				const tinygltf::BufferView& normBufferView = model.bufferViews[normAccessor.bufferView];
				normBuffer = reinterpret_cast<const float*>(&(model.buffers[normBufferView.buffer].data[normAccessor.byteOffset + normBufferView.byteOffset]));
			}

			// and parse uv data - there can be two tex coord buffers, we need to check for both
			const float *uvBuffer0 = nullptr;
			if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
			{
				const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
				const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
				uvBuffer0 = reinterpret_cast<const float*>(&(model.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
			}

			const float *uvBuffer1 = nullptr;
			if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
			{
				const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
				const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
				uvBuffer1 = reinterpret_cast<const float*>(&(model.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
			}

			// check whether this model has skinning data - joints first
			const uint16_t *jointBuffer = nullptr;
			if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
			{
				skinned = true;
				const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
				const tinygltf::BufferView& jointBufferView = model.bufferViews[jointAccessor.bufferView];
				jointBuffer = reinterpret_cast<const uint16_t*>(&(model.buffers[jointBufferView.buffer].data[jointAccessor.byteOffset + jointBufferView.byteOffset]));
			}

			// and then weights. It must contain both to for the data to be used for animations
			const float *weightBuffer = nullptr;
			if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
			{
				const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
				const tinygltf::BufferView& weightBufferView = model.bufferViews[weightAccessor.bufferView];
				weightBuffer = reinterpret_cast<const float*>(&(model.buffers[weightBufferView.buffer].data[weightAccessor.byteOffset + weightBufferView.byteOffset]));
			}

			// get the min and max values for this primitive TODO:: FIX THIS!
			OEMaths::vec3f primMin{ 0.0f, 0.0f, 0.0f }; // { posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2] };
			OEMaths::vec3f primMax{ 0.0f, 0.0f, 0.0f };  //{ posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2] };

			// now convert the data to a form that we can use with Vulkan - skinned and non-skinned meshes treated separately
			for (uint32_t j = 0; j < posAccessor.count; ++j)
			{
				Vertex vertex;
				vertex.position = OEMaths::vec4f(OEMaths::vec3f(posBuffer), 1.0f);
				posBuffer += 3;

				if (normBuffer)
				{
					vertex.normal = OEMaths::vec3f(normBuffer);
					normBuffer += 3;
				}
				if (uvBuffer0)
				{
					vertex.uv0 = OEMaths::vec2f(uvBuffer0);
					uvBuffer0 += 2;
				}
				if (uvBuffer1)
				{
					vertex.uv1 = OEMaths::vec2f(uvBuffer1);
					uvBuffer1 += 2;
				}

				// if we have skin data, also convert this to a palatable form
				if (weightBuffer && jointBuffer)
				{
					vertex.joint = OEMaths::vec4f(jointBuffer);
					vertex.weight = OEMaths::vec4f(weightBuffer);
					jointBuffer += 4;
					weightBuffer += 4;
				}

				vertices.push_back(vertex);
			}

			localVertexOffset += static_cast<uint32_t>(posAccessor.count);
			
			// Now obtain the indicies data from the gltf file
			const tinygltf::Accessor& indAccessor = model.accessors[primitive.indices];
			const tinygltf::BufferView& indBufferView = model.bufferViews[indAccessor.bufferView];
			const tinygltf::Buffer& indBuffer = model.buffers[indBufferView.buffer];

			uint32_t indexCount = static_cast<uint32_t>(indAccessor.count);

			// the indicies can be stored in various formats - this can be determined from the component type in the accessor
			switch (indAccessor.componentType)
			{
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
			{
				parseIndices<uint32_t>(indAccessor, indBufferView, indBuffer, indices, vertexStart);
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
			{
				parseIndices<uint16_t>(indAccessor, indBufferView, indBuffer, indices, vertexStart);
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
			{
				parseIndices<uint8_t>(indAccessor, indBufferView, indBuffer, indices, vertexStart);
				break;
			}
			default:
				throw std::runtime_error("Unable to parse indices data. Unsupported accessor component type.");
			}

			primitives.push_back({ localIndexOffset, indexCount, primMin, primMax, static_cast<uint32_t>(primitive.material) });
			localIndexOffset += indexCount;
		}
	}
}
