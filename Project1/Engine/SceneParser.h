#pragma once

#include "rapidjson/document.h"

using namespace rapidjson;

class SceneParser
{

public:

	SceneParser();
	~SceneParser();

	bool open(std::string filename);

private:

	Document document;
};

