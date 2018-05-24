#include "ObjectResourceManager.h"
#include "utility/serialisation.h"
#include "ComponentManagers/PhysicsComponentManager.h"
#include "ComponentManagers/TransformComponentManager.h"
#include "ComponentManagers/MeshComponentManager.h"


ObjectResourceManager::ObjectResourceManager()
{

}

ObjectResourceManager::ObjectResourceManager(World *world) :
	p_world(world)
{
}


ObjectResourceManager::~ObjectResourceManager()
{
}

void ObjectResourceManager::RegisterWorld(World *world)
{
	assert(world != nullptr);
	p_world = world;
}

void ObjectResourceManager::LoadObjectData(std::string filename)
{
	Archiver *archive = new Archiver();

	archive->OpenFile(filename, SaveMode::SAVE_TEXT, FileMode::FILE_IN);
	ObjectArchiver(archive, *p_world, Archiver::var_info(""));
}

void ObjectResourceManager::ObjectArchiver(Archiver *arch, World& data, const Archiver::var_info& info)
{
	MapObjectArchiver(arch, data.m_managers, info.name + "m_managers");
}

void ObjectResourceManager::MapObjectArchiver(Archiver *arch, std::unordered_map<std::type_index, ComponentManager*>& map, const Archiver::var_info& info)
{
	std::string type;
	uint32_t mapSize = static_cast<uint32_t>(map.size());
	arch->Serialise(mapSize, Archiver::var_info(info.name + ".size"));

	auto iter = map.begin();
	for (uint32_t c = 0; c < mapSize; ++c) {
		if (map.size() != mapSize) {			// if map sizes differ then data isn't present - so de-serialise data from file

			arch->Serialise(type, Archiver::var_info(info.name + "Key::[" + std::to_string(c) + "]"));
			std::type_index index = p_world->RegisterComponentManager(type);
			map[index]->Serialise(arch, *map.at(index), Archiver::var_info(info.name + "Data::[" + std::to_string(c) + "]"));
		}
		else {									// otherwise data is present in map, so serialise data to file
			// TODO :: de-serialising isn't possible yet - rectify this
			std::string name = "Need to implement!";	
			arch->Serialise(name, Archiver::var_info(info.name + "Key::[" + std::to_string(c) + "]"));
			iter->second->Serialise(arch, *iter->second, Archiver::var_info(info.name + "Data::[" + std::to_string(c) + "]"));
		}
		++iter;
	}
}
