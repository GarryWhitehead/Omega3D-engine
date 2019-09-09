#pragma once

#include <cstdint>

namespace Util
{
class String
{
public:
	String(const char* str);

	String(const String& str);
	String& operator=(const String& str);

	String(String&& str);
	String& operator=(String&& str);

	~String();

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
