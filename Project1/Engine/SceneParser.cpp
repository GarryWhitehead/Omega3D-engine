#include "SceneParser.h"
#include "DataTypes/Camera.h"
#include "DataTypes/Space.h"

#include "Engine/Omega_Global.h"
#include "Utility/FileUtil.h"
#include "Engine/World.h"

#include "rapidjson/stringbuffer.h"

namespace OmegaEngine
{

	SceneParser::SceneParser()
	{
	}


	SceneParser::~SceneParser()
	{
	}

	bool SceneParser::open(std::string filename)
	{
		std::string json;
		if (!FileUtil::readFileIntoBuffer(filename, json)) {
			return false;
		}


		if (document.Parse(json.c_str()).HasParseError()) {
			return false;
		}

		return true;
	}

	bool SceneParser::getCameraData(CameraDataType& cameraData)
	{
		// it's essential that the world doc has a camera type
		if (!document.HasMember("Camera")) {
			return false;
		}
		const Value& cam = document["Camera"];

		// read the camera values from the array
		cameraData.zNear = cam[0].GetFloat();
		cameraData.zFar = cam[1].GetFloat();
		cameraData.velocity = cam[2].GetFloat();
		cameraData.fov = cam[3].GetFloat();

		cameraData.position.x = cam[4].GetFloat();
		cameraData.position.y = cam[5].GetFloat();
		cameraData.position.z = cam[6].GetFloat();

		cameraData.cameraUp.x = cam[7].GetFloat();
		cameraData.cameraUp.y = cam[8].GetFloat();
		cameraData.cameraUp.z = cam[9].GetFloat();

		cameraData.type = (CameraType)cam[10].GetInt();
	}

	bool SceneParser::getWorldInfo(World::WorldInfo& worldInfo)
	{
		// it's essential that the world doc has a camera type
		if (document.HasMember("World Name")) {
			worldInfo.name = document["World Name"].GetString();
		}
		if (document.HasMember("World Width")) {
			worldInfo.width = document["World Width"].GetInt();
		}
		if (document.HasMember("World Height")) {
			worldInfo.height = document["World Height"].GetInt();
		}
		if (document.HasMember("World Size")) {
			worldInfo.totalSpaces = document["World Size"].GetInt();
		}

		return true;
	}

	bool SceneParser::getSceneFileList(std::vector<std::string>& spaceFilenames)
	{
		if (!document.HasMember("Space")) {
			return false;
		}

		// parse all the spaces out of the array
		const Value& spaceArray = document["Spaces"];
		if (spaceArray.Empty()) {
			return false;
		}

		spaceFilenames.resize(spaceArray.Size());
		for (uint32_t i = 0; i < spaceArray.Size(); ++i) {
			spaceFilenames[i] = spaceArray[i].GetString;
		}

		return true;
	}


}
