#pragma once

#include <string>
#include <vector>

namespace StringUtil
{
	std::string lastPart(std::string str, char identifier);
	std::vector<std::string> splitString(std::string str, char identifier);
}

