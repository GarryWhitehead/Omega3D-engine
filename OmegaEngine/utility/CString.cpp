#include "CString.h"

#include <assert.h>
#include <cstring>
#include <stdlib.h>
#include <vector>

namespace Util
{
String::String(const char* str)
{
	this->length = std::strlen(str);
	if (this->length > 0)
	{
		this->buffer = (char*)malloc(length + 1);
		memcpy(buffer, str, length);
		buffer[length] = '\0';
	}
}

String::String(const String& rhs)
    : String(rhs.buffer)
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
		String(rhs.buffer);
	}
	return *this;
}

String::String(String&& rhs)
    : buffer(std::exchange(rhs.buffer, nullptr))
    , length(std::exchange(rhs.length, 0))
{
}

String& String::operator=(String&& rhs)
{
	if (this != &rhs)
	{
		buffer = std::exchange(rhs.buffer, nullptr);
		length = std::exchange(rhs.length, 0);
	}
	return *this;
}

String::~String()
{
	if (buffer)
	{
		free(buffer);
		buffer = nullptr;
	}
}

bool String::compare(String str)
{
	assert(buffer);
	assert(str.buffer);

	int diff = std::strcmp(buffer, str.buffer);
	return diff == 0;
}

std::vector<String> String::split(String str, char identifier)
{
	if (str.empty())
	{
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
}    // namespace Util
