#pragma once
#include "utility/serialisation.h"
#include <vector>
#include <functional>
#include <typeindex>
#include "Engine/World.h"

class ObjectResourceManager
{

public:

	struct ComponentTypeHeader
	{
		std::vector<uint32_t> objectIndex;
	};

	struct ObjectHeader
	{
		uint32_t objectCount;
		uint32_t componentTypeCount;
		std::vector<ComponentTypeHeader> componentTypes;
	};

	ObjectResourceManager();
	ObjectResourceManager(World *world);
	~ObjectResourceManager();

	void RegisterWorld(World *world);
	void LoadObjectData(std::string filename);
	void ObjectArchiver(Archiver *arch, World& data, const Archiver::var_info& info);
	void MapObjectArchiver(Archiver *arch, std::unordered_map<std::type_index, ComponentManager*>& map, const Archiver::var_info& info);
	
private:

	World *p_world;

};

