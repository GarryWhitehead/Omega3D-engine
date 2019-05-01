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
			fprintf(stderr, "Error! The input string doesn't contain identifier %s.\n", identifier);
			return "";
		}

		std::string output = str.substr(pos + 1, str.size());
		printf("Output str = %s\n", output.c_str());
		return output;
	}
	
	std::string splitString(std::string str, char identifier, uint32_t part)
	{
		if (str.empty()) 
		{
			fprintf(stderr, "Error! Empty string given as parameter.\n");
			return "";
		}

		std::vector<std::string> split_strings;
		size_t pos = str.find_first_of(identifier);
		while (pos != std::string::npos)
		{
			std::string split = str.substr(0, pos);
			printf("Split string = %s\n", split.c_str());
			split_strings.emplace_back(split);
			str = str.substr(pos + 1, str.size());
			printf("New str = %s\n", str.c_str());
			pos = str.find_first_of(identifier);
		}

		if (split_strings.empty() || split_strings.size() < part)
		{
			return "";
		}

		return split_strings[part];
	}
}
