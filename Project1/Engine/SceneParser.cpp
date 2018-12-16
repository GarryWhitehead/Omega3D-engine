#include "SceneParser.h"

#include "rapidjson/stringbuffer.h"
#include "Omega_Global.h"
#include "Utility/FileManager.h"

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
