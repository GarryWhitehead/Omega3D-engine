#include "ObjectResourceManager.h"
#include "utility/serialisation.h"
#include "ComponentManagers/PhysicsComponentManager.h"
#include "ComponentManagers/TransformComponentManager.h"

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

	*g_filelog << "Preparing to open file : " << filename << "\n";
	archive->OpenFile(filename, SaveMode::SAVE_TEXT, FileMode::FILE_IN);

	*g_filelog << "File suceesfully opened. Importing data into world.......\n";
	ObjectArchiver(archive, *p_world, Archiver::var_info(""));
}

void ObjectResourceManager::ObjectArchiver(Archiver *arch, World& data, const Archiver::var_info& info)
{
	MapObjectArchiver(arch, data.m_managers, info.name + "m_managers");
}

void ObjectResourceManager::MapObjectArchiver(Archiver *arch, std::unordered_map<ComponentManagerId, ComponentManager*>& map, const Archiver::var_info& info)
{
	ComponentManagerId type;
	uint32_t mapSize = static_cast<uint32_t>(map.size());
	arch->Serialise(mapSize, Archiver::var_info(info.name + ".size"));

	auto iter = map.begin();
	for (uint32_t c = 0; c < mapSize; ++c) {
		if (map.size() != mapSize) {

			arch->Serialise((int&)type, Archiver::var_info(info.name + "Key::[" + std::to_string(c) + "]"));
			p_world->RegisterComponentManager(type);
			map[type]->Serialise(arch, *map.at(type), Archiver::var_info(info.name + "Data::[" + std::to_string(c) + "]"));
		}
		else {
			type = iter->first;
			arch->Serialise((int&)type, Archiver::var_info(info.name + "Key::[" + std::to_string(c) + "]"));
			iter->second->Serialise(arch, *iter->second, Archiver::var_info(info.name + "Data::[" + std::to_string(c) + "]"));
		}
		++iter;
	}
}
