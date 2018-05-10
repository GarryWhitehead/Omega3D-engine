#include "ComponentManagers/TransformComponentManager.h"
#include "ComponentManagers/PhysicsComponentManager.h"
#include "Engine/ObjectManager.h"
#include "Engine/World.h"
#include "Engine/engine.h"
#include <gtc/matrix_transform.hpp>

TransformComponentManager::TransformComponentManager(ComponentManagerId id) :
	ArchivableComponentManager<TransformComponentManager>(*this, id),
	dataUpdated(false)
{
}

TransformComponentManager::~TransformComponentManager()
{
	Destroy();
}

void TransformComponentManager::Init(World *world, ObjectManager *manager)
{
	p_world = world;
	p_objectManager = manager;

	RegisterWithManager(ComponentManagerId::CM_MESH_ID);
}

void TransformComponentManager::Update()
{
	auto manager = GetRegisteredManager<PhysicsComponentManager>(ComponentManagerId::CM_PHYSICS_ID);

	std::vector<PhysicsComponentManager::UpdatedPhysics> updatedData;
	manager->DownloadPhysicsData(updatedData);

	std::unordered_map<Object, uint32_t, HashGameObj> objectIndicies;
	manager->DownloadObjectIndex(objectIndicies);

	assert(updatedData.size() == objectIndicies.size());
	m_data.worldTransform.resize(objectIndicies.size());

	// ensure that there are objects and position data to transfrom with
	if (!updatedData.empty() && !objectIndicies.empty()) {

		for (auto &objIndex : objectIndicies) {

			// first do a check to ensure that the position and transform data are synced to the same object at a particular index
			Object obj = objIndex.first;
			uint32_t index = objIndex.second;
			if (m_indicies[obj] == index) {

				glm::mat4 trans(1.0);
				if (!updatedData[index].pos.empty()) {
					trans = glm::translate(trans, glm::vec3(updatedData[index].pos[0]));
				}
				if (!updatedData[index].rot.empty()) {
					glm::vec4 rot = updatedData[index].rot[0];
					trans = glm::rotate(trans, rot.w, glm::vec3(rot.x, rot.y, rot.z));
				}
				if (!updatedData[index].scale.empty()) {
					trans = glm::scale(trans, updatedData[index].scale[0]);
				}
				m_data.worldTransform[index] = trans;
			}
			else {
				*g_filelog << "Error updating transform data. Object indicies out of sync - Index 1: " << m_indicies[obj] << " , Index 2: " << objectIndicies[obj] << "\n";
			}

		}
	}

	dataUpdated = true;
}

void TransformComponentManager::SetLocalTransform(uint32_t index, glm::mat4 mat)
{
	m_data.localTransform[index] = mat;
	uint32_t parentIndex = m_data.parentIndex[index];
	glm::mat4 parentTransform = (parentIndex >= 0) ? m_data.worldTransform[parentIndex] : glm::mat4(1.0f);
	Transform(index, parentTransform);
}

void TransformComponentManager::Transform(uint32_t index, glm::mat4 parentMatrix)
{
	m_data.worldTransform[index] = m_data.localTransform[index] * parentMatrix;
	uint32_t childIndex = m_data.firstChildIndex[index];
	while (childIndex >= 0) {

		Transform(childIndex, m_data.worldTransform[index]);
		childIndex = m_data.nextChildIndex[childIndex];
	}
}

void TransformComponentManager::DownloadWorldTransformData(glm::mat4 transformData[256])
{ 
	if (!dataUpdated) {
		return;
	}

	for (int c = 0; c < m_data.worldTransform.size(); ++c) {

		transformData[c] = m_data.worldTransform[c];
	}
}

void TransformComponentManager::Destroy()
{

}

// Serialisation fucntions

void TransformComponentManager::Serialise(Archiver* arch, TransformComponentManager& manager, const Archiver::var_info& info)
{
	*g_filelog << "De/serialising data for transform component manager.......";
	arch->Serialise<uint32_t>(manager.m_data.firstChildIndex, Archiver::var_info(info.name + ".m_data.acceleration"));
	arch->Serialise<uint32_t>(manager.m_data.nextChildIndex, Archiver::var_info(info.name + ".m_data.mass"));
	arch->Serialise<uint32_t>(manager.m_data.parentIndex, Archiver::var_info(info.name + ".m_data.mass"));
	p_objectManager->Serialise(arch, manager.m_data.object, Archiver::var_info(info.name + ".m_data.object"));		// a custom specific serialiser is used for the vector objects
	*g_filelog << " Successfully imported!\n";

	// update object indicies with newly imported data. If the data is being serialised, then this won't have any effect as the object will already be present in the map
	uint32_t index = m_data.object.size() - 1;
	m_indicies.insert(std::make_pair(m_data.object[index], index));
}