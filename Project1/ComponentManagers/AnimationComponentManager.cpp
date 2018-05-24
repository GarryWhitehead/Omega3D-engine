#include "AnimationComponentManager.h"
#include "Engine/ObjectManager.h"


AnimationComponentManager::AnimationComponentManager() :
	ArchivableComponentManager<AnimationComponentManager>(*this)
{
}


AnimationComponentManager::~AnimationComponentManager()
{
}

void AnimationComponentManager::Init(World *world, ObjectManager *manager)
{
	p_world = world;
	p_objectManager = manager;
}

void AnimationComponentManager::Update()
{

}

void AnimationComponentManager::Destroy()
{

}

void AnimationComponentManager::ProcessBones()
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
}

void AnimationComponentManager::AddVertexBoneData(uint32_t index, uint32_t id, float weight)
{
	/* for (uint32_t b = 0; b < MAX_VERTEX_BONES; ++b) {

		if (m_vertexBoneData[index].weights[b] == 0.0f) {

			m_vertexBoneData[index].boneId[b] = id;
			m_vertexBoneData[index].weights[b] = weight;
			return;
		}
	} */
}

void AnimationComponentManager::UpdateModelAnimation()
{
	//float ticks = 25.0f * Engine::DT;
	//float animationTime = fmod(ticks, model->)
	//interpolate

	// the root global transform
	/*glm::mat4 parentTransform = glm::mat4(1.0f);
	glm::mat4 globalTransform = parentTransform * m_boneData[0].localTransform;
	m_boneData[0].finalTransform = m_globalInvTransform * globalTransform * m_boneData[0].invBind;
	parentTransform = globalTransform;

	uint32_t index = 0;
	uint32_t childSize = 0;

	while (index < 0 /*p_scene->skeletonData.bones.size() ) {

		for (int c = 0; p_scene->skeleton->skeletonData.bones[index].numChildren.size(); ++c) {

			childSize = p_scene->skeleton->skeletonData.bones[index].numChildren[c];
			uint32_t offset = p_scene->skeleton->skeletonData.bones[index].childrenInd[c];

			for (int i = 0; i < childSize; ++i) {

				std::string name = p_scene->skeleton->skeletonData.bones[i + offset].sid;
				if (m_boneMap.find(name) != m_boneMap.end()) {

					uint32_t mapIndex = m_boneMap[name];
					globalTransform = parentTransform * m_boneData[mapIndex].localTransform;
					m_boneData[mapIndex].finalTransform = m_globalInvTransform * globalTransform *m_boneData[mapIndex].invBind;
					parentTransform = globalTransform;
				}
			}
			index += childSize;
		}
	}

	for (int c = 0; c < m_boneData.size(); ++c) {

		m_boneTransforms[c] = m_boneData[c].finalTransform;
	} */
}

bool AnimationComponentManager::HasObject(Object& otherObj)
{
	if (m_data.object.empty()) {
		return false;
	}
	
	for (auto& obj : m_data.object) {

		if (obj.GetId() == otherObj.GetId()) {
			
			return true;
		}
	}
	return false;
}

void AnimationComponentManager::Serialise(Archiver* arch, AnimationComponentManager& manager, const Archiver::var_info& info)
{
	*g_filelog << "De/serialising data for animation component manager.......";

	arch->Serialise<float>(manager.m_data.time, Archiver::var_info(info.name + ".m_data.acceleration"));
	p_objectManager->Serialise(arch, manager.m_data.object, Archiver::var_info(info.name + ".m_data.object"));		// a custom specific serialiser is used for the vector objects

	*g_filelog << " Successfully imported!\n";
}