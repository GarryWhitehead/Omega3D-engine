#pragma once
#include <vector>
#include <unordered_map>
#include "glm.hpp"
#include "ComponentManagers/ComponentManager.h"
#include "Engine/Object.h"
#include <array>

class ObjectManager;
class World;

class TransformComponentManager : public ArchivableComponentManager<TransformComponentManager>
{

public:

	struct ComponentData
	{
		std::vector<Object> object;
		std::vector<glm::mat4> localTransform;
		std::vector<glm::mat4> worldTransform;
		std::vector<uint32_t> parentIndex;
		std::vector<uint32_t> firstChildIndex;
		std::vector<uint32_t> nextChildIndex;
		std::vector<uint32_t> modelIndex;
	};

	TransformComponentManager(ComponentManagerId id);
	~TransformComponentManager();

	void Init(World *world, ObjectManager *manager) override;
	void Update() override;
	void Destroy() override;
	void SetLocalTransform(uint32_t index, glm::mat4 mat);
	void Transform(uint32_t index, glm::mat4 mat);
	void Serialise(Archiver* arch, TransformComponentManager& manager, const Archiver::var_info& info);

	std::array<glm::mat4, 256> DownloadWorldTransformData();

private:

	ComponentData m_data;

	std::unordered_map<Object, uint32_t, HashGameObj> m_indicies;

	bool dataUpdated;
};

