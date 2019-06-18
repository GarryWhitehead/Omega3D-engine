#pragma once

#include <string>

namespace StringUtil
{
	std::string lastPart(std::string str, char identifier);
	std::vector<std::string> splitString(std::string str, char identifier);
}

