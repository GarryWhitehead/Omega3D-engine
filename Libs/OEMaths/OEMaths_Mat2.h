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

#include "OEMaths_MatN.h"
#include "OEMaths_Vec2.h"

#include <cstdint>

namespace OEMaths
{

template <typename T>
class MatN<T, 2, 2> : public MatMathOperators<MatN, T, 2, 2>
{
public:
    inline void setCol(size_t col, const VecN<T, 2>& vec)
    {
        assert(col < NUM_COLS);
        data[col] = vec;
    }

    // init as identity matrix
    MatN()
    {
        setCol(0, {1.0f, 0.0f});
        setCol(1, {0.0f, 1.0f});
    }

    inline constexpr VecN<T, 2>& operator[](const size_t& idx)
    {
        assert(idx < NUM_COLS);
        return data[idx];
    }

    inline constexpr VecN<T, 2> operator[](const size_t& idx) const
    {
        assert(idx < NUM_COLS);
        return data[idx];
    }

public:
    // ========= matrix transforms ==================

    inline constexpr MatN<T, 2, 2>& setDiag(const VecN<T, 2>& vec)
    {
        data[0][0] = vec.x;
        data[1][1] = vec.y;
        return *this;
    }

    static constexpr size_t NUM_ROWS = 2;
    static constexpr size_t NUM_COLS = 2;
    static constexpr size_t MAT_SIZE = 4;

public:
    /**
     * coloumn major - data can be accesed by [col][row] format.
     */
    VecN<T, NUM_ROWS> data[NUM_COLS];
};

using mat2f = MatN<float, 2, 2>;
using mat2d = MatN<double, 2, 2>;

} // namespace OEMaths
