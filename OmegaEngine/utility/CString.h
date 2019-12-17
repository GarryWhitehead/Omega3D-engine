#pragma once

#include <cstdint>
#include <cstddef>

namespace Util
{
class String
{
public:

	String() = default;
	String(const char* str);

	String(const String& str);
	String& operator=(const String& str);

	String(String&& str);
	String& operator=(String&& str);

	~String();

	/**
	* @brief A comapre two strings; a simple wrapper around the c-style
	* strncmp
	* @param str the string to compare with
	* @return A boolean indicating whether the two strings are identical
	*/
	bool compare(String str);

	bool empty() const
	{
		return !buffer;
	}

	size_t size() const
	{
		return length;
	}

	char* c_str() const
	{
		return buffer;
	}

private:
	char* buffer = nullptr;
	size_t length = 0;
};
}    // namespace Util
