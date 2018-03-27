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

private:

};

