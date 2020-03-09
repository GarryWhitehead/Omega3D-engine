#include "CString.h"

#include <assert.h>
#include <cstring>
#include <stdlib.h>
#include <vector>

namespace Util
{
String::String(const char* str)
{
	if (str)
	{
		length = std::strlen(str);
		if (this->length > 0)
		{
			buffer = (char*)malloc(length + 1);
			assert(buffer);
			memcpy(buffer, str, length);
			buffer[length] = '\0';
		}
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
		new (this) String(rhs.buffer);
	}
	return *this;
}

String::String(String&& rhs) noexcept
    : buffer(std::exchange(rhs.buffer, nullptr))
    , length(std::exchange(rhs.length, 0))
{
}

String& String::operator=(String&& rhs) noexcept
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

size_t String::operator()(const String& str) const
{

	size_t result = 2166136261;
	for (size_t i = 0; i < length; ++i)
	{
		result = (result * 16777619) ^ str.buffer[i];
	}

	return result;
}

bool String::compare(const String& str) const
{
	assert(buffer);
	assert(str.buffer);

	int diff = std::strcmp(buffer, str.buffer);
	return diff == 0;
}

float String::toFloat() const
{
	return std::atof(buffer);
}

uint32_t String::toUInt32() const
{
	return std::atol(buffer);
}

uint64_t String::toUInt64() const
{
	return std::atoll(buffer);
}

int String::toInt() const
{
	return std::atoll(buffer);
}

// =============== static functions =============================
std::vector<String> String::split(String str, char identifier)
{
	if (str.empty())
	{
		return {};
	}

	std::vector<String> result;

	char* bufferPtr = str.buffer;
	std::vector<char> splitBuffer;
	splitBuffer.reserve(str.length);

	for (size_t i = 0; i < str.length; ++i)
	{
		if (*bufferPtr == identifier)
		{
			if (!splitBuffer.empty())
			{
				splitBuffer.emplace_back('\0');
				String splitStr((const char*)splitBuffer.data());
				result.emplace_back(splitStr);
				splitBuffer.clear();
				++bufferPtr;
			}
			else
			{
				++bufferPtr;
				continue;
			}
		}
		splitBuffer.emplace_back(*bufferPtr++);
	}

	if (!splitBuffer.empty())
	{
		splitBuffer.emplace_back('\0');
		String splitStr((const char*)splitBuffer.data());
		result.emplace_back(splitStr);
	}

	return result;
}

Util::String String::append(const Util::String& lhs, const Util::String& rhs)
{
	if (!lhs.buffer)
	{
		return rhs;
	}
	if (rhs.empty())
	{
		return lhs;
	}

	Util::String newStr;
	size_t newLength = rhs.length + lhs.length;

	char* newBuffer = new char[newLength];
	memcpy(newBuffer, lhs.buffer, lhs.length);
	memcpy(newBuffer + lhs.length, rhs.buffer, rhs.length);
	newBuffer[newLength] = '\0';
	return String{ newBuffer };
}

}    // namespace Util
