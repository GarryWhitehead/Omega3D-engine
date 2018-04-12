#include "AnimationComponentManager.h"
#include "Engine/ObjectManager.h"


AnimationComponentManager::AnimationComponentManager(ComponentManagerId id) :
	ArchivableComponentManager<AnimationComponentManager>(*this, id)
{
}


AnimationComponentManager::~AnimationComponentManager()
{
}

void AnimationComponentManager::Init(World *world, ObjectManager *manager)
{
	p_world = world;
	p_objectManager = manager;

	// register with graphics system as the updated mesh data will be required for rendering
	//RegisterWithSystem(SystemId::GRAPHICS_SYSTEM_ID);

	RegisterWithManager(ComponentManagerId::CM_MESH_ID);
}

void AnimationComponentManager::Update()
{

}

void AnimationComponentManager::Destroy()
{

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