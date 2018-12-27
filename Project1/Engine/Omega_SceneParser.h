#pragma once

#include "rapidjson/document.h"

#include <string>
#include <vector>

#include "OEMaths/OEMaths.h"

using namespace rapidjson;

namespace OmegaEngine
{
	// forward declerations
	struct CameraDataType;
	struct Space;
	struct WorldInfo;
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

			// tells the scene manager the max number of spaces that can be loaded at one time. 
			// This is the stating a grid n*n - i.e. 3, 5, 7, 9, 11... Note: cannot be 1
			uint32_t gridSizeToLoad;
			uint32_t startingId;
			uint32_t totalSpaces;
		};

		struct ModelInfo
		{
			const char* gltfFilename;

			OEMaths::vec4f world_translation;
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

		bool loadCameraData();
		bool loadModels();
		bool loadWorldInfo();
		bool loadTerrainData();
		bool loadEnvironamnetData();
		bool loadLights();

	private:

		Document document;

		std::unique_ptr<CameraDataType> camera;
		std::unique_ptr<WorldInfo> worldInfo;

		std::vector<ModelInfo>& models;
	};

}

