#pragma once
#include "glm.hpp"
#include "ComponentManagers/ComponentManager.h"
#include "Engine/Object.h"

enum class LightType
{
	SPOT_LIGHT,
	DIRECTIONAL_LIGHT
};

class LightComponentManager : public ArchivableComponentManager<LightComponentManager>
{

public:

	LightComponentManager(ComponentManagerId id);
	~LightComponentManager();

	struct ComponentData
	{
		std::vector<Object> object;
		std::vector<LightType> type;
		std::vector<glm::vec4> pos;
		std::vector<glm::vec4> target;
		std::vector<glm::vec4> colour;
		std::vector<glm::mat4> viewMatrix;
	};

private:

	ComponentData data;
};

