#include "SceneParser.h"
#include "Camera.h"

#include "rapidjson/stringbuffer.h"
#include "Omega_Global.h"
#include "Utility/FileManager.h"

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
		if (OmegaEngine::Global::managers.fileManager->readFileIntoBuffer(filename, json)) {
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
		if (document.HasMember("Camera")) {
			return false;
		}
		const Value& cam = document["Camera"];

		// check that we have all the elements required to create a camera
		if (cam.Size() != CameraDataTypeElementCount) {
			return false;
		}

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

}
