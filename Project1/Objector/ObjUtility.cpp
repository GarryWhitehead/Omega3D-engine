#include "ObjUtility.h"
#include <sstream>
#include <algorithm>

ObjUtility::ObjUtility()
{
}


ObjUtility::~ObjUtility()
{
}


bool ObjUtility::RemoveCommentsFromStr(std::string& str)
{
	// comments in .obj files are denoted by the hash symbol
	// remove everything after and including the hash symbol
	str = str.substr(0, str.find('#'));

	return str.empty();
}

bool ObjUtility::CheckStringForForm(std::string str)
{
	size_t pos;
	// check whether the string contains v, vn, vt or vp
	pos = str.find("v");
	if (pos != std::string::npos) {
		return true;
	}

	// or conatins indices (face) data (f)
	pos = str.find("f");
	if (pos != std::string::npos) {
		return true;
	}
	return false;
}



