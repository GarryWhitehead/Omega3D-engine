#pragma once
#include "ComponentManagers/ComponentManager.h"
#include "Engine/Object.h"
#include <vector>

enum class ModelType;

class MeshComponentManager : public ArchivableComponentManager<MeshComponentManager>
{

public:

	struct ComponentData
	{
		std::vector<ModelType> type;
		std::vector<Object> object;
		std::vector<uint32_t> meshIndex;
	};

	MeshComponentManager(ComponentManagerId id);
	~MeshComponentManager();

	void Init(World *world, ObjectManager *manager) override;
	void Update() override;
	void Destroy() override;
	void Serialise(Archiver* arch, MeshComponentManager& manager, const Archiver::var_info& info);

	void InitModelTypes();
	void DownloadMeshIndicesData(std::vector<uint32_t>& staticIndicesData, std::vector<uint32_t>& animIndicesData);

private:

	ComponentData m_data;

	// a index loop-up map for identifying the index of entites within the component data
	std::unordered_map<Object, uint32_t, HashGameObj> m_indicies;
};

