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
			LOGGER_INFO("Scence .json file is not of the correct format.");
			return false;
		}

		// read the json now were all ready to go.
		// all data is optional. If not present then default values will be used
		loadCameraData();
		loadModels();
		loadLights();
		loadEnvironment();

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
		camera.start_position = OEMaths::vec3f(pos[0].GetFloat(), pos[1].GetFloat(), pos[2].GetFloat());

		auto &up = cam["CameraUp"];
		camera.camera_up = OEMaths::vec3f(up[0].GetFloat(), up[1].GetFloat(), up[2].GetFloat());

		std::string type = cam["Type"].GetString();
		if (type == "FPS") {
			camera.type = Camera::CameraType::FirstPerson;
		}
		else if (type == "Third Person") {
			camera.type = Camera::CameraType::ThirdPerson;
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
			models[i].world_rot = OEMaths::vec3f(rot[0].GetFloat(), rot[1].GetFloat(), rot[2].GetFloat());

			auto& sca = arr["Scale"];
			models[i].world_scale = OEMaths::vec3f(sca[0].GetFloat(), sca[1].GetFloat(), sca[2].GetFloat());

			auto& tran = arr["Translation"];
			models[i].world_translation = OEMaths::vec3f(tran[0].GetFloat(), tran[1].GetFloat(), tran[2].GetFloat());
		}
	}

	void SceneParser::loadTerrainData()
	{

	}

	void SceneParser::loadLights()
	{
		if (!document.HasMember("Lights")) {
			LOGGER_INFO("No lighting information found in scene file.");
			return;
		}

		const Value& lightArray = document["Lights"];
		if (lightArray.Empty()) {
			return;
		}

		lights.resize(lightArray.Size());

		for (uint32_t i = 0; i < lightArray.Size(); ++i) {
			auto& arr = lightArray[i];
			lights[i].type = static_cast<LightType>(arr["Type"].GetInt());

			auto& pos = arr["Position"];
			lights[i].postion = OEMaths::vec4f(pos[0].GetFloat(), pos[1].GetFloat(), pos[2].GetFloat(), pos[3].GetFloat());

			auto& col = arr["Colour"];
			lights[i].colour = OEMaths::vec3f(col[0].GetFloat(), col[1].GetFloat(), col[2].GetFloat());

			lights[i].radius = arr["Radius"].GetFloat();

			if (lights[i].type == LightType::Cone) {
				lights[i].innerCone = arr["InnerCone"].GetFloat();
				lights[i].outerCone = arr["OuterCone"].GetFloat();
			}
		}
	}

	void SceneParser::loadEnvironment()
	{
		// skybox
		if (document.HasMember("Skybox")) {
			environment.skybox_filename = document["Skybox"].GetString();
		}
		
		// bdrf and IBL
		if (document.HasMember("BRDF")) {
			environment.brdf_filename = document["BRDF"].GetString();
		}
		if (document.HasMember("IBL-Irradiance")) {
			environment.irradiance_map_filename = document["IBL-Irradiance"].GetString();
		}

	}


}
