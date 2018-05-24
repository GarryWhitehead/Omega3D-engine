#include "LightComponentManager.h"
#include "Engine/ObjectManager.h"
#include "Engine/engine.h"

LightComponentManager::LightComponentManager() :
	ArchivableComponentManager<LightComponentManager>(*this)
{
}


LightComponentManager::~LightComponentManager()
{
}

void LightComponentManager::Init(World *world, ObjectManager *manager)
{
	p_world = world;
	p_objectManager = manager;
}

void LightComponentManager::Update()
{
	for (uint32_t c = 0; c < m_data.lightInfo.size(); ++c) {

		m_data.lightInfo[c].pos.x += 2 * Engine::DT;
		m_data.lightInfo[c].pos.y -= 2 * Engine::DT;
		m_data.lightInfo[c].pos.z += 2 * Engine::DT;
	}
}

void LightComponentManager::Destroy()
{

}

void LightComponentManager::GetUpdatedLightInfo(std::array<LightComponentManager::LightInfo, MAX_LIGHT_COUNT>& info)
{
	for (uint32_t c = 0; c < m_data.lightInfo.size(); ++c) {

		info[c] = m_data.lightInfo[c];
	}
}

// Serialisation functions
void LightComponentManager::Serialise(Archiver* arch, LightInfo& light, const Archiver::var_info& info)
{
	arch->Serialise(light.colour, Archiver::var_info(info.name + ".colour"));
	arch->Serialise(light.fov, Archiver::var_info(info.name + ".fov"));
	arch->Serialise(light.pos, Archiver::var_info(info.name + ".pos"));
	arch->Serialise(light.target, Archiver::var_info(info.name + ".target"));
	arch->Serialise((uint32_t&)light.type, Archiver::var_info(info.name + ".type"));
}

void LightComponentManager::Serialise(Archiver *arch, std::vector<LightInfo>& vec, const Archiver::var_info& info)
{
	uint32_t vecSize = static_cast<uint32_t>(vec.size());
	arch->Serialise(vecSize, Archiver::var_info(info.name + ".size"));
	vec.resize(vecSize);

	for (uint32_t c = 0; c < vecSize; ++c) {

		Serialise(arch, vec[c], Archiver::var_info(info.name + "[" + std::to_string(c) + "]"));
	}
}

void LightComponentManager::Serialise(Archiver* arch, LightComponentManager& manager, const Archiver::var_info& info)
{
	*g_filelog << "De/serialising data for light component manager.......";
	Serialise(arch, manager.m_data.lightInfo, Archiver::var_info(info.name + ".m_data.lightInfo"));
	p_objectManager->Serialise(arch, manager.m_data.object, Archiver::var_info(info.name + ".m_data.object"));			// a custom specific serialiser is used for the vector objects

	*g_filelog << " Successfully imported!\n";

	// update object indicies with newly imported data. If the data is being serialised, then this won't have any effect as the object will already be present in the map
	uint32_t index = m_data.object.size() - 1;
	m_indicies.insert(std::make_pair(m_data.object[index], index));
}