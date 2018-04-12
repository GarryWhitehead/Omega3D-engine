#include "ModelInfo.h"
#include "VulkanCore/VulkanEngine.h"

ModelInfo::ModelInfo()
{
}

ModelInfo::ModelInfo(const ModelInfo& model) :
	vertices(model.vertices),
	indices(model.indices),
	meshData(model.meshData),
	materialData(model.materialData),
	vertexBuffer(model.vertexBuffer),
	indexBuffer(model.indexBuffer)
{
}

ModelInfo::~ModelInfo()
{
}

void ModelInfo::LoadModel(std::string filename, VulkanEngine *vkEngine, VkCommandPool cmdPool)
{
	Objector::ModelInfo *model = objector.ImportObjFile(filename);

	if (!objector.isObjImported()) {
		std::cout << "Error reading file: " << filename << "\n";
		exit(EXIT_FAILURE);
	}
	else {

		if (model->numMaterials > 0) {

			this->ProcessMaterials(model, vkEngine, cmdPool);
		}

		this->ProcessMeshes(model);
	}
}

void ModelInfo::ProcessMeshes(Objector::ModelInfo *model) 
{

	meshData.resize(model->numMeshes);
	uint32_t indexBase = 0;
	uint32_t vertexBase = 0;

	for (int c = 0; c < meshData.size(); ++c) {

		Objector::objMesh mesh = model->meshData[c];

		if (model->numMaterials > 0) {
			meshData[c].material = &materialData[mesh.materialIndex];
		}

		meshData[c].vertexBase = vertexBase;
		meshData[c].indexBase = indexBase;
		meshData[c].indexCount = mesh.numFaces() * mesh.faceData[0].numIndices();

		// upload vertices
		for (uint32_t i = 0; i < mesh.numPositions(); ++i) {
			ModelVertex vertex;

			if (mesh.hasPositions()) {
				vertex.pos = mesh.posData[i];
				vertex.pos.y = -vertex.pos.y;
			}
			else {

				vertex.pos = glm::vec3(0.0f);
			}

			if (mesh.hasUv()) {
				vertex.uv = mesh.uvData[i];
			}
			else {
				vertex.uv = glm::vec2(0.0f);
			}

			if (mesh.hasNormals()) {
				vertex.normal = mesh.normData[i];
				vertex.normal.y = -vertex.normal.y;						// flip for vulkan
			}
			else {
				vertex.normal = glm::vec3(0.0f);
			}

			if (mesh.hasColors()) {
				vertex.colour = mesh.colorData[i];
			}
			else {
				vertex.colour = glm::vec3(1.0f, 1.0f, 1.0f);
			}

			vertices.push_back(vertex);
		}

		vertexBase += vertices.size() * sizeof(ModelVertex);

		// retrieve the indices data
		for (uint32_t i = 0; i < mesh.numFaces(); ++i) {

			for (uint32_t v = 0; v < mesh.faceData[i].numIndices(); ++v) {

				indices.push_back(mesh.faceData[i].indices[v]);
			}
		}

		indexBase += mesh.numFaces() * 3;
	}

	// calculate size of each buffer
	vertexBuffer.size = sizeof(ModelInfo::ModelVertex) * vertices.size();
	indexBuffer.size = sizeof(uint32_t) * indices.size();
}

void ModelInfo::ProcessMaterials(Objector::ModelInfo *model, VulkanEngine *vkEngine, VkCommandPool cmdPool)
{
	materialData.resize(model->numMaterials);

	for (int c = 0; c < materialData.size(); ++c) {

		materialData[c] = {};

		objMaterial material = model->materials[c];

		// get material name
		materialData[c].name = material.name;

		// Get the material properties
		materialData[c].properties.ambient = glm::vec4(material.Color.ambient, 0.0f);
		materialData[c].properties.diffuse = glm::vec4(material.Color.diffuse, 0.0f);
		materialData[c].properties.specular = glm::vec4(material.Color.specular, 0.0f);
		materialData[c].properties.opacity = material.opacity;

		// load material textures and create texture in memory
		materialData[c].diffuse = LoadMaterialTexture(material, objTextureType::DIFFUSE_TEXTURE, vkEngine, cmdPool);
		materialData[c].specular = LoadMaterialTexture(material, objTextureType::SPECULAR_TEXTURE, vkEngine, cmdPool);
	}
}

TextureInfo ModelInfo::LoadMaterialTexture(objMaterial &material, objTextureType type, VulkanEngine *vkEngine, VkCommandPool cmdPool)
{
	if (!material.hasTextureType(type)) {

	}

	std::string filename = material.GetTextureTypeFilename(type);

	filename = filename.substr(0, filename.find(".png"));
	filename = filename + ".ktx";

	VulkanUtility vkUtility;
	vkUtility.InitVulkanUtility(vkEngine);
	TextureInfo texture = vkUtility.LoadTexture(filename, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_COMPARE_OP_ALWAYS, 16, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_R8G8B8A8_UNORM, cmdPool);

	return texture;
}

// vertex attributes for model vertex buffer

VkVertexInputBindingDescription ModelInfo::ModelVertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(ModelVertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

std::array<VkVertexInputAttributeDescription, 6> ModelInfo::ModelVertex::GetAttrBindingDescription()
{
	// Vertex layout 0: uv
	std::array<VkVertexInputAttributeDescription, 6> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(ModelVertex, pos);

	// Vertex layout 1: normal
	attrDescr[1].binding = 0;
	attrDescr[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[1].location = 1;
	attrDescr[1].offset = offsetof(ModelVertex, uv);

	// Vertex layout 2: colour
	attrDescr[2].binding = 0;
	attrDescr[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[2].location = 2;
	attrDescr[2].offset = offsetof(ModelVertex, normal);

	// Vertex layout 3: view vector
	attrDescr[3].binding = 0;
	attrDescr[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[3].location = 3;
	attrDescr[3].offset = offsetof(ModelVertex, colour);

	// NOT USED - only here as the vertex structs need to be the asme size for the shadow mapping to work correctly
	// Vertex layout 5: bone weights
	attrDescr[4].binding = 0;
	attrDescr[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attrDescr[4].location = 4;
	attrDescr[4].offset = offsetof(ModelVertex, boneWeigthts);

	// Vertex layout 6: bone ids
	attrDescr[5].binding = 0;
	attrDescr[5].format = VK_FORMAT_R32G32B32_SINT;
	attrDescr[5].location = 5;
	attrDescr[5].offset = offsetof(ModelVertex, boneId);

	return attrDescr;
}