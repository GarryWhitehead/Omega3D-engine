#pragma once
#include <vector>
#include <unordered_map>
#include "glm.hpp"
#include "ComponentManagers/ComponentManager.h"
#include "Engine/Object.h"

class ObjectManager;
class World;

enum class ModelType
{
	MODEL_STATIC,
	MODEL_ANIMATED
};

class TransformComponentManager : public ArchivableComponentManager<TransformComponentManager>
{

public:

	struct ComponentData
	{
		std::vector<ModelType> type;
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

	void DownloadWorldTransformData(std::vector<glm::mat4>& staticTransformData, std::vector<glm::mat4>& animTransformData);
	void UploadModelTypeData(std::vector<ModelType>& typeData);

private:

	ComponentData m_data;

	std::unordered_map<Object, uint32_t, HashGameObj> m_indicies;

	bool dataUpdated;
};

