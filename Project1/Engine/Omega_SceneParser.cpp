#include "Omega_SceneParser.h"
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

	bool SceneParser::parse(std::string filename)
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

	bool SceneParser::loadCameraData()
	{
		// it's essential that the world doc has a camera type
		if (!document.HasMember("Camera")) {
			return false;
		}
		const Value& cam = document["Camera"];
		
		camera = std::make_unique<CameraDataType>();

		// read the camera values from the array
		camera->zNear = cam["Znear"].GetFloat();
		camera->zFar = cam["ZFar"].GetFloat();
		camera->velocity = cam["Velocity"].GetFloat();
		camera->fov = cam["fov"].GetFloat();

		auto &pos = cam["Position"];
		camera->position.x = pos[0].GetFloat();
		camera->position.y = pos[1].GetFloat();
		camera->position.z = pos[2].GetFloat();

		auto &up = cam["CameraUp"];
		camera->cameraUp.x = up[0].GetFloat();
		camera->cameraUp.y = up[1].GetFloat();
		camera->cameraUp.z = up[2].GetFloat();

		std::string type = cam["Type"].GetString();
		if (type == "FPS") {
			camera->type = CameraType::FirstPerson;
		}
		else if (type == "Third Person") {
			camera->type = CameraType::ThirdPerson;
		}

		return true;
	}

	bool SceneParser::loadWorldInfo()
	{
		// it's essential that the world doc has a camera type
		if (document.HasMember("World Name")) {
			worldInfo->name = document["World Name"].GetString();
		}
		if (document.HasMember("World Width")) {
			worldInfo->width = document["World Width"].GetInt();
		}
		if (document.HasMember("World Height")) {
			worldInfo->height = document["World Height"].GetInt();
		}
		if (document.HasMember("World Size")) {
			worldInfo->totalSpaces = document["World Size"].GetInt();
		}

		return true;
	}

	bool SceneParser::loadModels()
	{
		if (!document.HasMember("Models")) {
			return false;
		}

		// parse all the spaces out of the array
		const Value& modelArray = document["Models"];
		if (modelArray.Empty()) {
			return false;
		}

		models.resize(modelArray.Size());
		for (uint32_t i = 0; i < modelArray.Size(); ++i) {
			auto& arr = modelArray[i];
			models[i].gltfFilename = arr["Filename"].GetString();
			
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
			models[i].world_translation.w = tran[3].GetFloat();
		}

		return true;
	}

	bool SceneParser::loadTerrainData()
	{

	}

	bool SceneParser::loadLights()
	{

	}

	bool SceneParser::loadEnvironamnetData()
	{

	}


}
