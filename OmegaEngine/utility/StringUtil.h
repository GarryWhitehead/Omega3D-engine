#pragma once

#include <string>

namespace StringUtil
{
	std::string lastPart(std::string str, char identifier);
	std::string splitString(std::string str, char identifier, uint32_t part);
}

