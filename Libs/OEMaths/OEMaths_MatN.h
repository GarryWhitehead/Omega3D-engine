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

#include <cstddef>
#include <cstdint>

namespace OEMaths
{

template <
    template <typename T, size_t cols, size_t rows>
    typename Mat,
    typename T,
    size_t cols,
    size_t rows>
class MatMathOperators
{
    // muliplication
    Mat<T, cols, rows>& operator*=(const T& div)
    {
        Mat<T, cols, rows>& lhs = static_cast<Mat<T, cols, rows>&>(&this);
        for (size_t i = 0; i < cols; ++i)
        {
            lhs[i] *= div;
        }
        return lhs;
    }

    inline friend Mat<T, cols, rows>
    operator*(const Mat<T, cols, rows>& m1, const Mat<T, cols, rows>& m2)
    {
        Mat<T, cols, rows> result;
        for (size_t j = 0; j < cols; ++j)
        {
            result[j] = m1 * m2[j];
        }
        return result;
    }

    inline friend VecN<T, rows> operator*(const Mat<T, cols, rows>& mat, const VecN<T, cols>& vec)
    {
        VecN<T, rows> result {T(0)};
        for (size_t j = 0; j < cols; ++j)
        {
            result += mat[j] * vec[j];
        }
        return result;
    }

    inline friend VecN<T, rows> operator*(const VecN<T, rows>& vec, const Mat<T, cols, rows>& mat)
    {
        VecN<T, rows> result {T(0)};
        for (size_t j = 0; j < cols; ++j)
        {
            result += vec[j] * mat[j];
        }
        return result;
    }

    inline friend VecN<T, rows> operator*(const Mat<T, cols, rows>& mat, const T& sca)
    {
        Mat<T, cols, rows> result;
        for (size_t j = 0; j < cols; ++j)
        {
            result[j] = mat[j] * sca;
        }
        return result;
    }

    // division
    Mat<T, cols, rows>& operator/=(const T& div)
    {
        Mat<T, cols, rows>& lhs = static_cast<Mat<T, cols, rows>&>(&this);
        for (size_t i = 0; i < cols; ++i)
        {
            lhs[i] /= div;
        }
        return lhs;
    }
};

template <typename T, size_t cols, size_t rows>
class MatN : public MatMathOperators<MatN, T, cols, rows>
{
public:
    // initialise to identity
    MatN()
    {
    }

    // copy
    MatN(const MatN<T, cols, rows>& other)
    {
        for (size_t i = 0; i < cols; ++i)
        {
            data[i] = other[i];
        }
    }

    // assignment
    inline MatN<T, cols, rows>& operator=(const MatN<T, cols, rows>& other)
    {
        if (this != &other)
        {
            MatN {other};
        }
        return *this;
    }

    inline constexpr MatN<T, cols, rows>& operator[](const size_t idx)
    {
        assert(idx < cols);
        return data[idx];
    }

    static constexpr size_t ROW_SIZE = rows;
    static constexpr size_t COL_SIZE = cols;
    static constexpr size_t MAT_SIZE = rows * cols;

public:
    VecN<T, ROW_SIZE> data[COL_SIZE];
};

// ================ matrix functions =====================


template <typename T, size_t cols, size_t rows>
MatN<T, cols, rows> transpose(MatN<T, cols, rows>& mat)
{
    MatN<T, cols, rows> result;
    for (size_t i = 0; i < cols; ++i)
    {
        for (size_t j = 0; j < rows; ++j)
        {
            result[i][j] = mat[j][i];
        }
    }
}

// ================= inverse functions ====================

/**
 * guass-jordan inverse function for matrices of any size
 * Use the faster versions below for 2x2 or 3x3 matrices
 */
template <typename T, size_t cols, size_t rows>
MatN<T, cols, rows> inverse(MatN<T, cols, rows>& mat)
{
    MatN<T, cols, rows> result;

    for (size_t i = 0; i < rows; ++i)
    {
        // look for largest element in i'th column
        size_t swap = i;
        T element = mat[i][i] < 0 ? -mat[i][i] : mat[i][i];
        for (size_t j = i + 1; j < rows; ++j)
        {
            const T element2 = mat[j][i] < 0 ? -mat[j][i] : mat[j][i];
            if (element2 > element)
            {
                swap = j;
                element = element2;
            }
        }

        if (swap != i)
        {
            // swap columns.
            std::swap(mat[i], mat[swap]);
            std::swap(result[i], result[swap]);
        }

        const T denom(mat[i][i]);
        for (size_t k = 0; k < rows; ++k)
        {
            mat[i][k] /= denom;
            result[i][k] /= denom;
        }

        // Factor out the lower triangle
        for (size_t j = 0; j < rows; ++j)
        {
            if (j != i)
            {
                const T t = mat[j][i];
                for (size_t k = 0; k < rows; ++k)
                {
                    mat[j][k] -= mat[i][k] * element;
                    result[j][k] -= result[i][k] * element;
                }
            }
        }
    }

    return result;
}


} // namespace OEMaths
