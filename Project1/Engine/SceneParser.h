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

	class SceneParser
	{

	public:

		// array element sizes
		const uint8_t CameraDataTypeElementCount = 11;

		SceneParser();
		~SceneParser();

		bool open(std::string filename);

		bool getCameraData(CameraDataType& camera);
		bool getSceneFileList(std::vector<std::string>& spaceFilenames);

	private:

		Document document;
	};

}

