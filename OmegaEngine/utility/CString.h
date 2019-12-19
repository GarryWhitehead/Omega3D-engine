#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <type_traits>

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

    static std::vector<String> split(String str, char identifier);
    
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
    
    /**
     * @brief Convert a numbert type to string
     */
    template <typename T>
    static String valueToString(T value)
    {
        char result[sizeof(T) + 2];
        if constexpr (std::is_same_v<T, float>)
        {
           sprintf(result, "%f", value);
        }
        else if constexpr (std::is_same_v<T, int>)
        {
           sprintf(result, "%d", value);
        }
        else if constexpr (std::is_same_v<T, uint64_t>)
        {
           sprintf(result, "%ul", value);
        }
        return String{result};
    }
    
    /**
     * @brief Converts a str to a value
     */
    template <typename T>
    static T stringToValue(String str)
    {
        T value;
        if constexpr (std::is_same_v<T, float>)
        {
            sprintnf(str.buffer, str.length, "%f", value);
        }
    }
    
private:
	char* buffer = nullptr;
	size_t length = 0;
};
}    // namespace Util
