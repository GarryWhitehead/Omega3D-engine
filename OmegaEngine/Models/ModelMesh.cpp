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

void ModelMesh::extractGltfMeshData(tinygltf::Model &model, tinygltf::Node &node)
{
	const tinygltf::Mesh &mesh = model.meshes[node.mesh];

	uint32_t localVertexOffset = 0;
	uint32_t localIndexOffset = 0;

	// get all the primitives associated with this mesh
	for (uint32_t i = 0; i < mesh.primitives.size(); ++i)
	{
		uint32_t vertexStart = static_cast<uint32_t>(vertices.size());
		const tinygltf::Primitive &primitive = mesh.primitives[i];

		// if this primitive has no indices associated with it, no point in continuing
		if (primitive.indices < 0)
		{
			continue;
		}

		// lets get the vertex data....
		if (primitive.attributes.find("POSITION") == primitive.attributes.end())
		{
			LOGGER_ERROR(
			    "Problem parsing gltf file. Appears to be missing position attribute. Exiting....");
		}

		const tinygltf::Accessor &posAccessor =
		    model.accessors[primitive.attributes.find("POSITION")->second];
		const tinygltf::BufferView &posBufferView = model.bufferViews[posAccessor.bufferView];
		const float *posBuffer = reinterpret_cast<const float *>(
		    &(model.buffers[posBufferView.buffer]
		          .data[posAccessor.byteOffset + posBufferView.byteOffset]));

		// find normal data
		const float *normBuffer = nullptr;
		if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
		{
			const tinygltf::Accessor &normAccessor =
			    model.accessors[primitive.attributes.find("NORMAL")->second];
			const tinygltf::BufferView &normBufferView = model.bufferViews[normAccessor.bufferView];
			normBuffer = reinterpret_cast<const float *>(
			    &(model.buffers[normBufferView.buffer]
			          .data[normAccessor.byteOffset + normBufferView.byteOffset]));
		}

		// and parse uv data - there can be two tex coord buffers, we need to check for both
		const float *uvBuffer0 = nullptr;
		if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
		{
			const tinygltf::Accessor &uvAccessor =
			    model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
			const tinygltf::BufferView &uvBufferView = model.bufferViews[uvAccessor.bufferView];
			uvBuffer0 = reinterpret_cast<const float *>(
			    &(model.buffers[uvBufferView.buffer]
			          .data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
		}

		const float *uvBuffer1 = nullptr;
		if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
		{
			const tinygltf::Accessor &uvAccessor =
			    model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
			const tinygltf::BufferView &uvBufferView = model.bufferViews[uvAccessor.bufferView];
			uvBuffer1 = reinterpret_cast<const float *>(
			    &(model.buffers[uvBufferView.buffer]
			          .data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
		}

		// check whether this model has skinning data - joints first
		const uint16_t *jointBuffer = nullptr;
		if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
		{
			skinned = true;
			const tinygltf::Accessor &jointAccessor =
			    model.accessors[primitive.attributes.find("JOINTS_0")->second];
			const tinygltf::BufferView &jointBufferView =
			    model.bufferViews[jointAccessor.bufferView];
			jointBuffer = reinterpret_cast<const uint16_t *>(
			    &(model.buffers[jointBufferView.buffer]
			          .data[jointAccessor.byteOffset + jointBufferView.byteOffset]));
		}

		// and then weights. It must contain both to for the data to be used for animations
		const float *weightBuffer = nullptr;
		if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
		{
			const tinygltf::Accessor &weightAccessor =
			    model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
			const tinygltf::BufferView &weightBufferView =
			    model.bufferViews[weightAccessor.bufferView];
			weightBuffer = reinterpret_cast<const float *>(
			    &(model.buffers[weightBufferView.buffer]
			          .data[weightAccessor.byteOffset + weightBufferView.byteOffset]));
		}

		// get the min and max values for this primitive TODO:: FIX THIS!
		OEMaths::vec3f primMin{
			0.0f, 0.0f, 0.0f
		}; // { posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2] };
		OEMaths::vec3f primMax{
			0.0f, 0.0f, 0.0f
		}; //{ posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2] };

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
		const tinygltf::Accessor &indAccessor = model.accessors[primitive.indices];
		const tinygltf::BufferView &indBufferView = model.bufferViews[indAccessor.bufferView];
		const tinygltf::Buffer &indBuffer = model.buffers[indBufferView.buffer];

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
			throw std::runtime_error(
			    "Unable to parse indices data. Unsupported accessor component type.");
		}

		primitives.push_back({ localIndexOffset, indexCount, static_cast<int32_t>(primitive.material) });
		localIndexOffset += indexCount;
	}
}

void ModelMesh::generatePlaneMesh(const uint32_t size, const uint32_t uvFactor)
{
	const float widthX = 3.0f;
	const float widthY = 3.0f;

	const uint32_t vertCount = size * size;
	vertices.reserve(vertCount);

	for (uint32_t x = 0; x < size; ++x)
	{
		for (uint32_t y = 0; y < size; ++y)
		{
			Vertex vertex;

			uint32_t index = x + y * size;
			vertex.position =
			    OEMaths::vec4f{ x * widthX + widthX / 2.0f - static_cast<float>(size) * widthX / 2.0f, 0.0f,
				                y * widthY + widthY / 2.0f - static_cast<float>(size) * widthY / 2.0f, 1.0f };
			vertex.uv0 = OEMaths::vec2f{ x / static_cast<float>(size), y / static_cast<float>(size) } *
			            uvFactor;

			vertices.emplace_back(vertex);
		}
	}

	// generate the indices for the patch quads
	uint32_t width = size - 1;
	uint32_t indicesSize = width * width * 4;

	indices.resize(indicesSize);

	for (uint32_t x = 0; x < width; ++x)
	{
		for (uint32_t y = 0; y < width; ++y)
		{
			uint32_t index = (x + y * width) * 4;
			indices[index] = (x + y * size); // top-left
			indices[index + 1] = indices[index] + size; // bottom-left
			indices[index + 2] = indices[index + 1] + 1; //	bottom-right
			indices[index + 3] = indices[index] + 1; // top-right
		}
	}

	primitives.push_back({ 0, static_cast<uint32_t>(indices.size()), -1 });
}

void ModelMesh::generateSphereMesh(const uint32_t density)
{
	vertices.reserve(6 * density * density);
	indices.reserve(2 * density * density * 6);

	static const OEMaths::vec3f basePosition[6] = {
		OEMaths::vec3f(1.0f, 1.0f, 1.0f),   OEMaths::vec3f(-1.0f, 1.0f, -1.0f),
		OEMaths::vec3f(-1.0f, 1.0f, -1.0f), OEMaths::vec3f(-1.0f, -1.0f, +1.0f),
		OEMaths::vec3f(-1.0f, 1.0f, +1.0f), OEMaths::vec3f(+1.0f, 1.0f, -1.0f),
	};

	static const OEMaths::vec3f dx[6] = {
		OEMaths::vec3f(0.0f, 0.0f, -2.0f), OEMaths::vec3f(0.0f, 0.0f, +2.0f),
		OEMaths::vec3f(2.0f, 0.0f, 0.0f),  OEMaths::vec3f(2.0f, 0.0f, 0.0f),
		OEMaths::vec3f(2.0f, 0.0f, 0.0f),  OEMaths::vec3f(-2.0f, 0.0f, 0.0f),
	};

	static const OEMaths::vec3f dy[6] = {
		OEMaths::vec3f(0.0f, -2.0f, 0.0f), OEMaths::vec3f(0.0f, -2.0f, 0.0f),
		OEMaths::vec3f(0.0f, 0.0f, +2.0f), OEMaths::vec3f(0.0f, 0.0f, -2.0f),
		OEMaths::vec3f(0.0f, -2.0f, 0.0f), OEMaths::vec3f(0.0f, -2.0f, 0.0f),
	};

	const float densityMod = 1.0f / static_cast<float>(density - 1);

	for (uint32_t face = 0; face < 6; ++face)
	{
		uint32_t indexOffset = face * density * density;

		for (uint32_t y = 0; y < density; ++y)
		{
			for (uint32_t x = 0; x < density; ++x)
			{
				Vertex vertex;
				vertex.uv0 = OEMaths::vec2f{ densityMod * x, densityMod * y };
				vertex.position =
				    OEMaths::vec4f{basePosition[face] + dx[face] * vertex.uv0.getX() + dy[face] * vertex.uv0.getY(), 1.0f};
				vertex.position.normalise();
				vertices.emplace_back(vertex);
			}
		}

		uint32_t strips = density - 1;
		for (uint32_t y = 0; y < strips; ++y)
		{
			uint32_t baseIndex = indexOffset + y * density;
			for (uint32_t x = 0; x < density; ++x)
			{
				indices.emplace_back(baseIndex + x);
				indices.emplace_back(baseIndex + x + density);
			}
		}
	}

	primitives.push_back({ 0, static_cast<uint32_t>(indices.size()), -1 });
}

void ModelMesh::generateCubeMesh(const OEMaths::vec3f& size)
{
	const float x = size.getX() / 2.0f;
	const float y = size.getY() / 2.0f;
	const float z = size.getZ() / 2.0f;

	// cube vertices
	static const std::array<OEMaths::vec3f, 8> vertexData{
		OEMaths::vec3f{ +x, +y, +z },
		OEMaths::vec3f{ -x, +y, +z },
		OEMaths::vec3f{ -x, -y, +z },
		OEMaths::vec3f{ +x, -y, +z },
		OEMaths::vec3f{ +x, +y, -z },
		OEMaths::vec3f{ -x, +y, -z },
		OEMaths::vec3f{ -x, -y, -z },
		OEMaths::vec3f{ +x, -y, -z }
	};

	// cube uv
	static const std::array<OEMaths::vec2f, 8> uvData
	{ 
		OEMaths::vec2f{ 1.0f, 1.0f },                                              
		OEMaths::vec2f{ 0.0f, 1.0f }, 
		OEMaths::vec2f{ 0.0f, 0.0f }, 
		OEMaths::vec2f{ 1.0f, 0.0f },                                   
		OEMaths::vec2f{ 0.0f, 1.0f },                                    
		OEMaths::vec2f{ 1.0f, 1.0f },
		OEMaths::vec2f{ 0.0f, 0.0f }, 
		OEMaths::vec2f{ 1.0f, 0.0f }
	};

	// cube indices
	static const std::array<uint32_t, 36> indexData{ // front
		                                           1, 2, 3, 3, 0, 1,
		                                           // right side
		                                           2, 6, 7, 7, 3, 2,
		                                           // back
		                                           6, 5, 4, 4, 7, 6,
		                                           // left side
		                                           5, 1, 0, 0, 4, 5,
		                                           // bottom
		                                           0, 3, 7, 7, 4, 0,
		                                           // top
		                                           5, 6, 2, 2, 1, 5
	};

	// generate normals
	/*for (int i = 0; i < 36; i += 3)
	{
		glm::vec3 normal =
		    glm::normalize(glm::cross(glm::vec3(verts[i + 1]) - glm::vec3(verts[i]),
		                              glm::vec3(verts[i + 2]) - glm::vec3(verts[i])));

		norm[i] = normal;
		norm[i + 1] = normal;
		norm[i + 2] = normal;
	}*/

	for (uint32_t i = 0; i < vertexData.size(); ++i)
	{
		Vertex vertex;
		vertex.position = OEMaths::vec4f{ vertexData[i], 1.0f };
		vertex.uv0 = uvData[i];
		vertices.emplace_back(vertex);
	}

	primitives.push_back({ 0, static_cast<uint32_t>(indices.size()), -1 });
}

} // namespace OmegaEngine
