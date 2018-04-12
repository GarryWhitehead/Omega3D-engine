#include "MeshComponentManager.h"
#include "ComponentManagers/TransformComponentManager.h"
#include "ComponentManagers/AnimationComponentManager.h"
#include "Engine/ObjectManager.h"
#include "Engine/engine.h"

MeshComponentManager::MeshComponentManager(ComponentManagerId id) :
	ArchivableComponentManager<MeshComponentManager>(*this, id)
{
}


MeshComponentManager::~MeshComponentManager()
{
}

void MeshComponentManager::Init(World *world, ObjectManager *manager)
{
	p_world = world;
	p_objectManager = manager;

	// register with graphics system as the updated mesh data will be required for rendering
	RegisterWithSystem(SystemId::GRAPHICS_SYSTEM_ID);
}

void MeshComponentManager::Update()
{

}

void MeshComponentManager::InitModelTypes()
{
	m_data.type.resize(m_data.object.size());

	// if animation manager isn't present, then assume all models are static
	if (!HasRegisteredManager(ComponentManagerId::CM_ANIMATION_ID)) {
		
		for (int c = 0; c < m_data.object.size(); ++c) {
			m_data.type[c] = ModelType::MODEL_STATIC;
		}
	}
	else {
		auto animManager = GetRegisteredManager<AnimationComponentManager>(ComponentManagerId::CM_ANIMATION_ID);

		for (int c = 0; c < m_data.object.size(); ++c) {

			if (animManager->HasObject(m_data.object[c])) {

				// if the object is linked with the animation manager, then this objects model must be animated and thus linked with the collada vulkan animation module
				m_data.type[c] = ModelType::MODEL_ANIMATED;
			}
			else {
				// not linked with the animation manager so assumed to be a static model
				m_data.type[c] = ModelType::MODEL_STATIC;
			}
		}
	}

	// the transform manager also requires this data
	auto transManager = GetRegisteredManager<TransformComponentManager>(ComponentManagerId::CM_TRANSFORM_ID);
	transManager->UploadModelTypeData(m_data.type);
}

void MeshComponentManager::DownloadMeshIndicesData(std::vector<uint32_t>& staticIndicesData, std::vector<uint32_t>& animIndicesData)
{
	for (int c = 0; c < m_data.object.size(); ++c) {

		if (m_data.type[c] == ModelType::MODEL_STATIC) {
			staticIndicesData.push_back(m_data.meshIndex[c]);
		}
		else if (m_data.type[c] == ModelType::MODEL_ANIMATED) {
			animIndicesData.push_back(m_data.meshIndex[c]);
		}
	}
}

void MeshComponentManager::Destroy()
{

}

// Serialisation functions
void MeshComponentManager::Serialise(Archiver* arch, MeshComponentManager& manager, const Archiver::var_info& info)
{
	*g_filelog << "De/serialising data for physics component manager.......";

	arch->Serialise<uint32_t>(manager.m_data.meshIndex, Archiver::var_info(info.name + ".m_data.meshIndex"));
	p_objectManager->Serialise(arch, manager.m_data.object, Archiver::var_info(info.name + ".m_data.object"));		// a custom specific serialiser is used for the vector objects

	*g_filelog << " Successfully imported!\n";

	// update object indicies with newly imported data. If the data is being serialised, then this won't have any effect as the object will already be present in the map
	uint32_t index = m_data.object.size() - 1;
	m_indicies.insert(std::make_pair(m_data.object[index], index));
}