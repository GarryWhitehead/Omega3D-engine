#include "StringUtil.h"
#include <vector>

namespace StringUtil
{
std::string lastPart(std::string str, char identifier)
{
	if (str.empty())
	{
		fprintf(stderr, "Error! Empty string given as parameter.\n");
		return "";
	}

	size_t pos = str.find_last_of(identifier);
	if (pos == std::string::npos)
	{
		fprintf(stderr, "Error! The input string doesn't contain identifier.\n");
		return "";
	}

	std::string output = str.substr(pos + 1, str.size());
	printf("Output str = %s\n", output.c_str());
	return output;
}

std::vector<std::string> splitString(std::string str, char identifier)
{
	if (str.empty())
	{
		fprintf(stderr, "Error! Empty string given as parameter.\n");
		return {};
	}

	std::vector<std::string> splitStrings;
	size_t pos = str.find_first_of(identifier);
	while (pos != std::string::npos)
	{
		std::string split = str.substr(0, pos);

		splitStrings.emplace_back(split);
		str = str.substr(pos + 1, str.size());

		pos = str.find_first_of(identifier);
	}

	// also include what's left after removing all identifiers
	splitStrings.emplace_back(str);

	return splitStrings;
}
} // namespace StringUtil
