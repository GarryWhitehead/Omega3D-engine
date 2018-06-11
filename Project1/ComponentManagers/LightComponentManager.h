#pragma once
#include "glm.hpp"
#include "ComponentManagers/ComponentManager.h"
#include "Engine/Object.h"
#include <array>

enum class LightType
{
	SPOT_LIGHT,
	DIRECTIONAL_LIGHT,
	OMNI_LIGHT
};

class LightComponentManager : public ArchivableComponentManager<LightComponentManager>
{

public:

	static const uint32_t MAX_LIGHT_COUNT = 200;

	struct LightInfo
	{
		LightType type;
		glm::vec4 pos;
		glm::vec4 target;
		glm::vec4 colour;
		float fov;
	};

	struct ComponentData
	{
		std::vector<Object> object;
		std::vector<LightInfo> lightInfo;
	};

	LightComponentManager();
	virtual ~LightComponentManager();

	void Init(World *world, ObjectManager *manager) override;
	void Update() override;
	void Destroy() override;
	void GetUpdatedLightInfo(std::array<LightInfo, MAX_LIGHT_COUNT>& info);

	// helper functions
	uint32_t GetLightCount() const { return m_data.lightInfo.size(); }
	float GetLightFOV(const uint32_t index) const { return m_data.lightInfo[index].fov; }
	LightInfo GetLightData(const uint32_t index) const { return m_data.lightInfo[index]; }

	// serialisation functions 
	void Serialise(Archiver* arch, LightComponentManager& manager, const Archiver::var_info& info);
	void Serialise(Archiver* arch, LightInfo& manager, const Archiver::var_info& info);
	void Serialise(Archiver* arch, std::vector<LightInfo>& vec, const Archiver::var_info& info);

private:

	ComponentData m_data;

	// a index loop-up map for identifying the index of entites within the component data
	std::unordered_map<Object, uint32_t, HashGameObj> m_indicies;
};


