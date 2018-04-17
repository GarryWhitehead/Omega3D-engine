#pragma once
#include <vector>
#include "glm.hpp"
#include "SiimpleCollada/XMLparse.h"

enum class TransformType
{
	TRANSFORM_FLOAT,
	TRANSFORM_4X4
};


class ColladaAnimation
{

public:

	struct LibraryAnimationFloatInfo
	{
		std::vector<float> time;
		std::vector<float> transform;
		std::vector<std::string> interpolation;
	};

	struct LibraryAnimation4X4Info
	{
		std::vector<float> time;
		std::vector<glm::mat4> transform;
		std::vector<std::string> interpolation;
	};

	struct InputSemanticInfo
	{
		std::string input;
		std::string output;
		std::string interpolation;
	};

	ColladaAnimation(XMLparse *parse);
	~ColladaAnimation();

	void ImportLibraryAnimations();

private:

	void GetAnimationDataFloatTransform(XMLPbuffer &buffer);
	void GetAnimationData4X4Transform(XMLPbuffer &buffer);
	uint32_t GetFloatArrayInfo(std::string id, XMLPbuffer& buffer, uint32_t index, uint32_t semanticIndex);
	
	XMLparse *m_xmlParse;

	TransformType transformType;
	std::vector<LibraryAnimationFloatInfo> libraryAnimationFloatData;
	std::vector<LibraryAnimation4X4Info> libraryAnimation4x4Data;

	std::vector<InputSemanticInfo> m_semanticData;
};

