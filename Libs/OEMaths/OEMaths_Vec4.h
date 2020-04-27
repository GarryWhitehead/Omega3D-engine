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

#include "OEMaths_VecN.h"

#include <cstdint>

namespace OEMaths
{
template <typename T>
class VecN<T, 4> : public MathOperators<VecN, T, 4>
{
public:
    VecN() : x(T(0)), y(T(0)), z(T(0)), w(T(0))
    {
    }

    VecN(const T& n) : x(n), y(n), z(n), w(n)
    {
    }

    VecN(const T& in_x, const T& in_y, const T& in_z, const T& in_w)
        : x(in_x), y(in_y), z(in_z), w(in_w)
    {
    }

    VecN(const VecN<T, 3>& vec, const T& value) : x(vec.x), y(vec.y), z(vec.z), w(value)
    {
    }

    inline constexpr T& operator[](const size_t idx)
    {
        assert(idx < 4);
        return data[idx];
    }

    inline constexpr T operator[](const size_t idx) const
    {
        assert(idx < 4);
        return data[idx];
    }

public:
    union
    {
        T data[4];

        VecN<T, 3> xyz;
        VecN<T, 3> stu;
        VecN<T, 3> rgb;
        VecN<T, 2> xy;
        VecN<T, 2> st;
        VecN<T, 2> rg;

        struct
        {
            T x;
            T y;
            T z;
            T w;
        };

        struct
        {
            T r;
            T g;
            T b;
            T a;
        };
    };
};

using vec4f = VecN<float, 4>;
using vec4d = VecN<double, 4>;
using vec4u16 = VecN<uint16_t, 4>;
using vec4u32 = VecN<uint32_t, 4>;
using vec4u64 = VecN<uint64_t, 4>;
using vec4i16 = VecN<int16_t, 4>;
using vec4i32 = VecN<int32_t, 4>;
using vec4i64 = VecN<int64_t, 4>;

// colour storage
using colour4 = VecN<float, 4>;

} // namespace OEMaths
