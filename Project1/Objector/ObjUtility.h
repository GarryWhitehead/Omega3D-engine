#pragma once
#include <string>
#include "glm.hpp"

class ObjUtility
{

public:

	ObjUtility();
	~ObjUtility();

	bool RemoveCommentsFromStr(std::string& str);
	bool CheckStringForForm(std::string str);
	glm::vec3 ConvertStrToVec3(std::string str);
	glm::vec2 ConvertStrToVec2(std::string str);
	std::string FilenameFromString(std::string str);

private:

};

