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

	String(String&& str) noexcept;
	String& operator=(String&& str) noexcept;
    
    bool operator==(const String& rhs) const noexcept;
    
	~String();

	/**
	* @brief A comapre two strings; a simple wrapper around the c-style
	* strncmp
	* @param str the string to compare with
	* @return A boolean indicating whether the two strings are identical
	*/
    bool compare(String str) const;

    float toFloat() const;
    uint32_t toUInt32() const;
    uint64_t toUInt64() const;
    int toInt() const;
    
	bool empty() const
	{
		return !buffer;
	}

	uint32_t size() const
	{
		return length;
	}

	char* c_str() const
	{
		return buffer;
	}
    
    // ================== static functions ==========================
    static std::vector<String> split(String str, char identifier);
    
    /**
     * @brief Appends a string to the end of the string held by the 'this' buffer
     */
    static Util::String append(Util::String lhs, Util::String rhs);

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
           sprintf(result, "%i", value);
        }
        return String{result};
    }
    
private:
    
	char* buffer = nullptr;
	uint32_t length = 0;
    
};

}    // namespace Util

namespace std
{
    template<>
    struct std::hash<Util::String>
    {
        size_t operator()(Util::String const& str) const noexcept
        {
            return (std::hash<const char*>()(str.c_str()));
        }
    };
}

