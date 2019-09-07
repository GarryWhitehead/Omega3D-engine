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

private:
	uint8_t* buffer = nullptr;
	size_t length = 0;
};
}    // namespace Util
