#pragma once

#include "rapidjson/document.h"

#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "OEMaths/OEMaths.h"
#include <string>
#include <vector>

using namespace rapidjson;

namespace OmegaEngine
{
// forward declerations
struct Model;

class SceneParser
{

public:
	struct ModelInfo
	{
		std::string gltfFilename;

		OEMaths::vec3f worldTranslation;
		OEMaths::vec3f worldScale;
		OEMaths::vec3f worldRotation;
	};

	struct Terrain
	{
	};

	struct EnvironmentInfo
	{
		const char *skyboxFilename = nullptr;
		const char *brdfFilename = nullptr;
		const char *irradianceMapFilename = nullptr;
	};

	SceneParser();
	~SceneParser();

	bool parse(std::string filename);

	void loadCameraData();
	void loadModels();
	void loadTerrainData();
	void loadEnvironment();
	void loadLights();

	std::string &getFilenames(uint32_t index)
	{
		return models[index].gltfFilename;
	}

	uint32_t modelCount() const
	{
		return static_cast<uint32_t>(models.size());
	}

	OEMaths::mat4f getWorldMatrix(uint32_t index)
	{
		return OEMaths::mat4f::translate(models[index].worldTranslation) *
		       OEMaths::mat4f::scale(
		           models[index].worldScale); // *OEMaths::vec4f(models[index].worldRotation, 1.0f);
	}

	Camera &getCamera()
	{
		return camera;
	}

	uint32_t lightCount() const
	{
		return static_cast<uint32_t>(lights.size());
	}

	LightInfo &getLights(const uint32_t index)
	{
		return lights[index];
	}

	EnvironmentInfo &getEnvironment()
	{
		return environment;
	}

private:
	Document document;

	const char *name = nullptr;

	// TODO: allow multiple camera to be loaded
	Camera camera;
	EnvironmentInfo environment;

	std::vector<ModelInfo> models;
	std::vector<LightInfo> lights;
};

} // namespace OmegaEngine
