#include "ColladaModelInfo.h"
#include "utility/file_log.h"
#include "VulkanCore/VulkanEngine.h"
#include "Engine/engine.h"

ColladaModelInfo::ColladaModelInfo()
{
}


ColladaModelInfo::~ColladaModelInfo()
{
}

void ColladaModelInfo::ImportFile(std::string filename, VulkanEngine *vkEngine, VkCommandPool cmdPool)
{
	p_colladaImporter = new SimpleCollada();

	if (!p_colladaImporter->OpenColladaFile(filename)) {

		*g_filelog << "Critical error! Unable to open collada file " << filename << " Exiting......";
		exit(EXIT_FAILURE);
	}

	p_scene = p_colladaImporter->ImportColladaData();

	ProcessMaterials(vkEngine, cmdPool);
	ProcessMeshes();
}

void ColladaModelInfo::ProcessMeshes()
{	
	
	/*for (uint32_t m = 0; m < p_scene->meshData->meshes.size(); ++m) {

		// setup bone data
	//	m_globalInvTransform = glm::mat4(1.0f);		// global transform = node transform
	//	glm::inverse(m_globalInvTransform);
	//	m_vertexBoneData.resize(p_scene->meshData->meshes[m].position.size());

	//	uint32_t boneIndex = 0;
		// collect required skeleton bone data - inverse bind matrix and name id and weights per vertex

	//	for (uint32_t c = 0; c < p_scene->numMeshes(); ++c) {

	//		std::string boneName = p_scene->skeleton->skeletonData.bones[c].sid;
	//		uint32_t index = p_scene->meshData->skinData[m].FindBone(boneName);

	//		if (boneName != "") {

				if (m_boneMap.find(boneName) == m_boneMap.end()) {

					SkeletonInfo bone;
					bone.name = p_scene->meshData->skinData[m].joints[index];				// data for the bones is actually found in the skinning data - library_controllers
					bone.invBind = p_scene->meshData->skinData[m].invBind[index];
					bone.localTransform = p_scene->skeleton->skeletonData.GetLocalTransform(bone.name);
					m_boneData.push_back(bone);

					m_boneMap.insert(std::make_pair(boneName, boneIndex));
					++boneIndex;
				}
				else {
					boneIndex = m_boneMap[boneName];
				}
			}

			for (uint32_t w = 0; w < p_scene->skeleton->skeletonData.bones[index].numWeights(); ++w) {		// indices data in the format offset 0 = joint id; 1 = weight 

				uint32_t vertexIndex = p_scene->skeleton->skeletonData.bones[index].weights[w].vertexId;
				assert(vertexIndex < m_vertexBoneData.size());
				AddVertexBoneData(vertexIndex, boneIndex, p_scene->skeleton->skeletonData.bones[index].weights[w].weight);
			}
		}
	}
	m_boneTransforms.resize(m_boneData.size());
	*/
	uint32_t indexBase = 0;
	uint32_t vertexBase = 0;
	uint32_t count = 0;
	m_meshData.resize(p_scene->numMeshes());

	for (int c = 0; c < p_scene->numMeshes(); ++c) {			

		ColladaVertices::Mesh mesh = p_scene->meshData->meshes[c];
		m_meshData[c].vertexBase = vertexBase;				

		// upload vertices
		for (uint32_t i = 0; i < mesh.numPositions(); ++i) {
			
			ModelVertex vertex;
			vertex.pos = mesh.position[i];
			vertex.pos.y = -vertex.pos.y;
			vertex.uv = mesh.texcoord[i];
			vertex.normal = mesh.normal[i];
			vertex.normal.y = -vertex.normal.y;						// flip for vulkan

			if (mesh.hasColors()) {
				vertex.colour = mesh.color[i];
			}
			else {
				vertex.colour = glm::vec3(1.0f, 1.0f, 1.0f);
			}


			// add the bone info
			/*for (uint32_t b = 0; b < MAX_VERTEX_BONES; ++b) {
				vertex.boneId[b] = m_vertexBoneData[i].boneId[b];
				vertex.boneWeigthts[b] = m_vertexBoneData[i].weights[b];
			}*/

			vertices.push_back(vertex);
			++count;
		}

		m_meshData[c].vertexCount = count;
		vertexBase += count * sizeof(ModelVertex);

		// retrieve the face indices data for this mesh 
		for (uint32_t f = 0; f < mesh.numFaces(); ++f) {
			
			count = 0;
			FaceInfo face;

			// material for each face
			std::string mat = p_scene->meshData->meshes[c].face[f].material;
			face.materialIndex = FindMaterialIndex(mat);

			// indices data
			face.indexBase = indexBase;

			for (uint32_t v = 0; v < mesh.face[f].numIndices(); ++v) {

				indices.push_back(mesh.face[f].indices[v]);
				++count;
			}

			indexBase += count;
			face.indexCount = count;
			m_meshData[c].faceInfo.push_back(face);
		}
	}
	
	// calculate size of each buffer
	vertexBuffer.size = sizeof(ColladaModelInfo::ModelVertex) * vertices.size();
	indexBuffer.size = sizeof(uint32_t) * indices.size();
}

void ColladaModelInfo::AddVertexBoneData(uint32_t index, uint32_t id, float weight)
{
	for (uint32_t b = 0; b < MAX_VERTEX_BONES; ++b) {

		if (m_vertexBoneData[index].weights[b] == 0.0f) {

			m_vertexBoneData[index].boneId[b] = id;
			m_vertexBoneData[index].weights[b] = weight;
			return;
		}
	}
}

uint8_t ColladaModelInfo::FindMaterialIndex(std::string matName)
{
	uint32_t index = 0;
	for (auto& mat : m_materials) {

		if (mat.name == matName) {

			return index;
		}
		++index;
	}
	return UINT8_MAX;
}

void ColladaModelInfo::ProcessMaterials(VulkanEngine *vkEngine, VkCommandPool cmdPool)
{
	m_materials.resize(p_scene->materials->materials.size());

	for (int c = 0; c < m_materials.size(); ++c) {

		m_materials[c] = {};

		ColladaMaterials::MaterialInfo mat = p_scene->materials->materials[c];

		// get material name
		m_materials[c].name = mat.name;

		// Get the material properties
		m_materials[c].properties.ambient = glm::vec4(mat.ambient);
		m_materials[c].properties.diffuse = glm::vec4(mat.diffuse);
		m_materials[c].properties.specular = glm::vec4(mat.specular);
		m_materials[c].properties.transparency = mat.transparency;
		m_materials[c].properties.shininess = mat.shininess;

		if (!mat.hasTexture()) {
			m_materials[c].matTypes |= (int)MaterialTexture::TEX_NONE;
			m_materials[c].texture.nomap = LoadMaterialTexture(mat, MatType::MAT_NONE, vkEngine, cmdPool);
		}
		else {
			if (mat.hasTexture(MatType::MAT_DIFFUSE)) {
				m_materials[c].texture.diffuse = LoadMaterialTexture(mat, MatType::MAT_DIFFUSE, vkEngine, cmdPool);
				m_materials[c].matTypes |= (int)MaterialTexture::TEX_DIFF;
			}
			if (mat.hasTexture(MatType::MAT_NORMAL)) {
				m_materials[c].texture.normal = LoadMaterialTexture(mat, MatType::MAT_NORMAL, vkEngine, cmdPool);
				m_materials[c].matTypes |= (int)MaterialTexture::TEX_NORM;
			}
		}
	}
}

TextureInfo ColladaModelInfo::LoadMaterialTexture(ColladaMaterials::MaterialInfo &material,  MatType type, VulkanEngine *vkEngine, VkCommandPool cmdPool)
{
	
	std::string filename;

	if (type == MatType::MAT_NONE) {

		filename = "dummy.ktx";
	}
	else {

		filename = material.GetMaterialType(type);

		// check whether original file is of tga format
		size_t pos = filename.find(".tga");
		if (pos != std::string::npos) {

			filename = filename.substr(0, pos);			// TODO:: add jpg, png, tga
			filename = filename + ".ktx";
		}
		pos = filename.find(".jpg");
		if (pos != std::string::npos) {

			filename = filename.substr(0, pos);			// TODO:: add jpg, png, tga
			filename = filename + ".ktx";
		}
	}

	VulkanUtility vkUtility;
	vkUtility.InitVulkanUtility(vkEngine);

	filename = "assets/" + filename;
	TextureInfo texture = vkUtility.LoadTexture(filename, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_COMPARE_OP_ALWAYS, 16, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE, VK_FORMAT_BC3_UNORM_BLOCK, cmdPool);

	return texture;
}

void ColladaModelInfo::UpdateModelAnimation()
{
	//float ticks = 25.0f * Engine::DT;
	//float animationTime = fmod(ticks, model->)
	//interpolate

	// the root global transform
	glm::mat4 parentTransform = glm::mat4(1.0f);
	glm::mat4 globalTransform = parentTransform * m_boneData[0].localTransform; 
	m_boneData[0].finalTransform = m_globalInvTransform * globalTransform * m_boneData[0].invBind;
	parentTransform = globalTransform;

	uint32_t index = 0;
	uint32_t childSize = 0;

	while (index < 0 /*p_scene->skeletonData.bones.size() */) {

		for (int c = 0; p_scene->skeleton->skeletonData.bones[index].numChildren.size(); ++c) {
			
			childSize = p_scene->skeleton->skeletonData.bones[index].numChildren[c];
			uint32_t offset = p_scene->skeleton->skeletonData.bones[index].childrenInd[c];

			for (int i = 0; i < childSize; ++i) {

				std::string name = p_scene->skeleton->skeletonData.bones[i + offset].sid;
				if (m_boneMap.find(name) != m_boneMap.end()) {

					uint32_t mapIndex = m_boneMap[name];
					globalTransform = parentTransform * m_boneData[mapIndex].localTransform;
					m_boneData[mapIndex].finalTransform =  m_globalInvTransform * globalTransform *m_boneData[mapIndex].invBind;
					parentTransform = globalTransform;
				}
			}
			index += childSize;
		}
	}

	for (int c = 0; c < m_boneData.size(); ++c) {

		m_boneTransforms[c] = m_boneData[c].finalTransform;
	}
}


// vertex attributes for model vertex buffer

VkVertexInputBindingDescription ColladaModelInfo::ModelVertex::GetInputBindingDescription()
{
	VkVertexInputBindingDescription bindDescr = {};
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(ModelVertex);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindDescr;
}

std::array<VkVertexInputAttributeDescription, 6> ColladaModelInfo::ModelVertex::GetAttrBindingDescription()
{
	// Vertex layout 0: pos
	std::array<VkVertexInputAttributeDescription, 6> attrDescr = {};
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(ModelVertex, pos);

	// Vertex layout 1: uv
	attrDescr[1].binding = 0;
	attrDescr[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[1].location = 1;
	attrDescr[1].offset = offsetof(ModelVertex, uv);

	// Vertex layout 2: normal
	attrDescr[2].binding = 0;
	attrDescr[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[2].location = 2;
	attrDescr[2].offset = offsetof(ModelVertex, normal);

	// Vertex layout 4: colour
	attrDescr[3].binding = 0;
	attrDescr[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrDescr[3].location = 3;
	attrDescr[3].offset = offsetof(ModelVertex, colour);

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