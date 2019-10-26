#pragma once

#include "OEMaths_VecN.h"

#include <cstddef>
#include <cstdint>

namespace OEMaths
{

template <template <typename T, size_t cols, size_t rows> typename Mat, typename T, size_t cols, size_t rows>
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

	Mat<T, cols, rows> operator*(const Mat<T, cols, rows>& m1, const Mat<T, cols, rows>& m2)
	{
		Mat<T, cols, rows> result;

		for (size_t j = 0; j < rows; ++j)
		{
			for (size_t i = 0; i < cols; ++i)
			{
				result[j * rows + i] = 0;

				for (size_t k = 0; k < cols; ++k)
				{
					result[i][j] += m1[j][k] * m2[k][i];
				}
			}
		}

		return result;
	}

	inline friend VecN<T, rows> operator*(const Mat<T, cols, rows>& mat, const VecN<T, cols>& vec)
	{
		VecN<T, rows> result{ T(0) };
        for (size_t i = 0; i < cols; ++i)
        {
            result[j] += mat[i] * vec;
        }
		return result;
	}

	inline friend VecN<T, rows> operator*(const VecN<T, rows>& vec, const Mat<T, cols, rows>& mat)
	{
		VecN<T, rows> result{ T(0) };
		for (size_t j = 0; j < rows; ++j)
		{
			for (size_t i = 0; i < cols; ++i)
			{
				result[j] += vec[i] * mat[i][j];
			}
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
			MatN{ other };
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

template <typename T, size_t cols, size_t rows, typename U>
MatN<T, cols, rows> makeMatrix(U* data)
{
    assert(data != nullptr);
    MatN<T, cols, rows> result;
    U* ptr = static_cast<U*>(data);

    for (size_t i = 0; i < cols; ++i)
    {
        result[i] = makeVector(*ptr);
        ++ptr;
    }
}

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

/**
 * Extracts a quaternin from a matrix type. This code is identical to the code in Eigen.
 */
template <typename T, size_t cols, size_t rows>
Quaternion<T> matToQuat(MatN<T, cols, rows>& mat)
{
    Quaternion<T> quat;

    // Compute the trace to see if it is positive or not.
    const T trace = mat[0][0] + mat[1][1] + mat[2][2];

    // check the sign of the trace
    if (trace > 0)
    {
        // trace is positive
        T s = std::sqrt(trace + 1);
        quat.w = T(0.5) * s;
        s = T(0.5) / s;
        quat.x = (mat[1][2] - mat[2][1]) * s;
        quat.y = (mat[2][0] - mat[0][2]) * s;
        quat.z = (mat[0][1] - mat[1][0]) * s;
    }
    else
    {
        // trace is negative
        // Find the index of the greatest diagonal
        size_t i = 0;
        if (mat[1][1] > mat[0][0])
        {
            i = 1;
            
        }
        if (mat[2][2] > mat[i][i])
        {
            i = 2;
        }

        // Get the next indices: (n+1)%3
        static constexpr size_t next_ijk[3] = { 1, 2, 0 };
        size_t j = next_ijk[i];
        size_t k = next_ijk[j];
        T s = std::sqrt((mat[i][i] - (mat[j][j] + mat[k][k])) + 1);
        quat[i] = T(0.5) * s;
        if (s != 0) {
            s = T(0.5) / s;
        }
        quat.w  = (mat[j][k] - mat[k][j]) * s;
        quat[j] = (mat[i][j] + mat[j][i]) * s;
        quat[k] = (mat[i][k] + mat[k][i]) * s;
    }
    return quat;
}
}
 
