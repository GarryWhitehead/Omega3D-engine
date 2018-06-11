#pragma once

#include "ComponentManagers/ComponentManager.h"
#include "Engine/Object.h"
#include <array>

static const int MAX_VERTEX_BONES = 4;

class AnimationComponentManager : public ArchivableComponentManager<AnimationComponentManager>
{

public:

	struct Component
	{
		std::vector<Object> object;
		std::vector<float> time;
	};

	struct SkeletonInfo
	{
		std::string name;
		glm::mat4 invBind;
		glm::mat4 localTransform;
		glm::mat4 finalTransform;
	};

	struct VertexBoneInfo
	{
		std::array<float, MAX_VERTEX_BONES> weights;
		std::array<uint32_t, MAX_VERTEX_BONES> boneId;
	};

	AnimationComponentManager();
	virtual ~AnimationComponentManager();

	void Init(World *world, ObjectManager *manager) override;
	void Update() override;
	void Destroy() override;
	void ProcessBones();
	void AddVertexBoneData(uint32_t index, uint32_t id, float weight);
	void UpdateModelAnimation();

	bool HasObject(Object& obj);
	void Serialise(Archiver* arch, AnimationComponentManager& manager, const Archiver::var_info& info);

private:

	Component m_data;
};

