#pragma once

#include "rapidjson/document.h"

#include <string>
#include <vector>

using namespace rapidjson;

namespace OmegaEngine
{
	// forward declerations
	struct CameraDataType;
	struct Space;
	struct WorldInfo;

	class SceneParser
	{

	public:

		SceneParser();
		~SceneParser();

		bool open(std::string filename);

		bool getCameraData(CameraDataType& camera);
		bool getSceneFileList(std::vector<std::string>& spaceFilenames);
		bool getWorldInfo(World::WorldInfo& worldInfo);

	private:

		Document document;
	};

}

