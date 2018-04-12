#pragma once

#include "ComponentManagers/ComponentManager.h"
#include "Engine/Object.h"

class AnimationComponentManager : public ArchivableComponentManager<AnimationComponentManager>
{

public:

	struct Component
	{
		std::vector<Object> object;
		std::vector<float> time;
	};

	AnimationComponentManager(ComponentManagerId id);
	~AnimationComponentManager();

	void Init(World *world, ObjectManager *manager) override;
	void Update() override;
	void Destroy() override;

	bool HasObject(Object& obj);
	void Serialise(Archiver* arch, AnimationComponentManager& manager, const Archiver::var_info& info);

private:

	Component m_data;
};

