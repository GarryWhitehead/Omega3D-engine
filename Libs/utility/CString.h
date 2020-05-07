/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

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
    bool operator!=(const String& rhs) const noexcept;

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
        return String {result};
    }

private:
    char* buffer = nullptr;
    uint32_t length = 0;
};

} // namespace Util

namespace std
{
template <>
struct std::hash<Util::String>
{
    size_t operator()(Util::String const& str) const noexcept
    {
        return (std::hash<const char*>()(str.c_str()));
    }
};
} // namespace std
