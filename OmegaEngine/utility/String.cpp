#include "String.h"

#include <cstring>
#include <stdlib.h>

namepsace Util
{
	String::String(const char* str)
	{
		this->length = std::strlen(str);
		if (this->length > 0)
		{
			this->buffer = (uint8_t*)malloc(len + 1);
			memcpy(string, str, len);
			string[len] = '\0';
		}
	}

	String::String(const String& rhs)
	    : String(rhs.c_str())
	{
	}

	String& String::operator=(const String& rhs)
	{
		if (this != &rhs)
			{
			if (buffer)
			{
				free(buffer);
			}
			String(rhs);
		}
	}

	String::String(String && str)
	{
	}
	String& String::operator=(String&& str)
	{
	}

	String::~String()
	{
		if (buffer)
		{
			free(buffer);
			buffer = nullptr;
		}
	}

	std::vector<String> String::split(String str, char identifier)
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
}
