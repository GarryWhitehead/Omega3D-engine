#pragma once
#include <vector>
#include <unordered_map>
#include "glm.hpp"
#include "ComponentManagers/ComponentManager.h"
#include "Engine/Object.h"

class ObjectManager;
class World;

class PhysicsComponentManager : public ArchivableComponentManager<PhysicsComponentManager>
{

public:

	// data that is to be updated is stored in this struct. Empty vectors signal that this particular method isnt updated
	struct UpdatedPhysics
	{
		std::vector<glm::vec3> pos;
		std::vector<glm::vec4> rot;
		std::vector<glm::vec3> scale;
	};

	// this assumes that all entities are stored at identical indicies within each buffer
	struct ComponentData
	{
		std::vector<Object> object;
		std::vector<glm::vec3> position;
		std::vector<glm::vec4> rotation;		// rotation axis stored in x,y,z; roation amount storted in w
		std::vector<glm::vec3> scale;
		std::vector<glm::vec3> velocity;
		std::vector<glm::vec3> acceleration;
		std::vector<float> mass;
	};

	PhysicsComponentManager(ComponentManagerId id);
	~PhysicsComponentManager();
	
	void Init(World *world, ObjectManager *manager) override;
	void Update() override;
	void Destroy() override;
	void Serialise(Archiver *arch, PhysicsComponentManager& component, const Archiver::var_info& info);

	void DownloadPhysicsData(std::vector<UpdatedPhysics>& updatedPos);
	void DownloadObjectIndex(std::unordered_map<Object, uint32_t, HashGameObj>& objectIndex);

private:

	ComponentData m_data;

	// a index loop-up map for identifying the index of entites within the component data
	std::unordered_map<Object, uint32_t, HashGameObj> m_indicies;
};

