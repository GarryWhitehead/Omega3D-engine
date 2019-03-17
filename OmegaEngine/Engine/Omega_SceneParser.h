#pragma once

#include "rapidjson/document.h"

#include <string>
#include <vector>
#include "Managers/CameraManager.h"
#include "Managers/LightManager.h"
#include "OEMaths/OEMaths.h"

using namespace rapidjson;

namespace OmegaEngine
{
	// forward declerations
	struct Model;

	class SceneParser
	{

	public:

		struct WorldInfo
		{
			const char* name;

			// this refers to the scene grid dimensions 
			uint32_t width;
			uint32_t height;
		};

		struct ModelInfo
		{
			std::string gltfFilename;

			OEMaths::vec3f world_translation;
			OEMaths::vec3f world_scale;
			OEMaths::vec3f world_rot;

		};

		struct Environment
		{
			const char* skybox_filename;
		};

		struct Terrain
		{

		};

		SceneParser();
		~SceneParser();

		bool parse(std::string filename);

		void loadCameraData();
		void loadModels();
		void loadWorldInfo();
		void loadTerrainData();
		void loadEnvironamnetData();
		void loadLights();

		std::string& getFilenames(uint32_t index)
		{
			return models[index].gltfFilename;
		}

		uint32_t modelCount() const
		{
			return static_cast<uint32_t>(models.size());
		}

		OEMaths::mat4f getWorldMatrix(uint32_t index)
		{
			return OEMaths::translate_mat4(models[index].world_translation) * OEMaths::scale_mat4(models[index].world_scale);	// *OEMaths::vec4f(models[index].world_rot, 1.0f);
		}

		Camera& get_camera()
		{
			return camera;
		}

		uint32_t light_count() const
		{
			return lights.size();
		}

		LightInfo& get_light(const uint32_t index) 
		{
			return lights[index];
		}

	private:

		Document document;

		// TODO: allow multiple camera to be loaded
		Camera camera;
		WorldInfo worldInfo;

		std::vector<ModelInfo> models;
		std::vector<LightInfo> lights;
	};

}

