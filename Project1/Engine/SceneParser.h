#pragma once

#include "rapidjson/document.h"

using namespace rapidjson;

namespace OmegaEngine
{
	// forward declerations
	struct CameraDataType;

	class SceneParser
	{

	public:

		// array element sizes
		const uint8_t CameraDataTypeElementCount = 11;

		SceneParser();
		~SceneParser();

		bool open(std::string filename);

		bool getCameraData(CameraDataType& camera);

	private:

		Document document;
	};

}

