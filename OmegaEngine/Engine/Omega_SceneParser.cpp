#include "Omega_SceneParser.h"

#include "Engine/Omega_Global.h"
#include "Utility/FileUtil.h"
#include "Utility/Logger.h"
#include "Engine/World.h"
#include "Managers/CameraManager.h"
#include "rapidjson/stringbuffer.h"

namespace OmegaEngine
{

	SceneParser::SceneParser()
	{
	}


	SceneParser::~SceneParser()
	{
	}

	bool SceneParser::parse(std::string filename)
	{
		std::string json;
		if (!FileUtil::readFileIntoBuffer(filename, json)) {
			return false;
		}

		if (document.Parse(json.c_str()).HasParseError()) {
			return false;
		}

		// read the json now were all ready to go.
		// all data is optional. If not present then default values will be used
		loadCameraData();
		loadWorldInfo();
		loadModels();
		loadLights();

		return true;
	}

	void SceneParser::loadCameraData()
	{
		if (!document.HasMember("Camera")) {
			LOGGER_INFO("No camera found in scene file. Using default calibration.");
			return;
		}
		const Value& cam = document["Camera"];

		// read the camera values from the array
		camera.zNear = cam["Znear"].GetFloat();
		camera.zFar = cam["ZFar"].GetFloat();
		camera.velocity = cam["Velocity"].GetFloat();
		camera.fov = cam["fov"].GetFloat();

		auto &pos = cam["Position"];
		camera.start_position.x = pos[0].GetFloat();
		camera.start_position.y = pos[1].GetFloat();
		camera.start_position.z = pos[2].GetFloat();

		auto &up = cam["CameraUp"];
		camera.camera_up.x = up[0].GetFloat();
		camera.camera_up.y = up[1].GetFloat();
		camera.camera_up.z = up[2].GetFloat();

		std::string type = cam["Type"].GetString();
		if (type == "FPS") {
			camera.type = Camera::CameraType::FirstPerson;
		}
		else if (type == "Third Person") {
			camera.type = Camera::CameraType::ThirdPerson;
		}
	}

	void SceneParser::loadWorldInfo()
	{

		if (document.HasMember("World Name")) {
			worldInfo.name = document["World Name"].GetString();
		}
		if (document.HasMember("World Width")) {
			worldInfo.width = document["World Width"].GetInt();
		}
		if (document.HasMember("World Height")) {
			worldInfo.height = document["World Height"].GetInt();
		}
		
	}

	void SceneParser::loadModels()
	{
		if (!document.HasMember("Models")) {
			LOGGER_INFO("No model data found in scene file. Assuming using object component system to fill world?");
			return;
		}

		// parse all the spaces out of the array
		const Value& modelArray = document["Models"];
		if (modelArray.Empty()) {
			return;
		}

		// all models should reside in the directory assets/textures/...
		std::string dir = "assets/textures/";

		models.resize(modelArray.Size());
		for (uint32_t i = 0; i < modelArray.Size(); ++i) {
			auto& arr = modelArray[i];
			models[i].gltfFilename = dir + arr["Filename"].GetString();
			
			auto& rot = arr["Rotation"];
			models[i].world_rot.x = rot[0].GetFloat();
			models[i].world_rot.y = rot[1].GetFloat();
			models[i].world_rot.z = rot[2].GetFloat();

			auto& sca = arr["Scale"];
			models[i].world_scale.x = sca[0].GetFloat();
			models[i].world_scale.y = sca[1].GetFloat();
			models[i].world_scale.z = sca[2].GetFloat();

			auto& tran = arr["Translation"];
			models[i].world_translation.x = tran[0].GetFloat();
			models[i].world_translation.y = tran[1].GetFloat();
			models[i].world_translation.z = tran[2].GetFloat();
		}
	}

	void SceneParser::loadTerrainData()
	{

	}

	void SceneParser::loadLights()
	{

	}

	void SceneParser::loadEnvironamnetData()
	{

	}


}
