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
	Mat<T, cols, rows>& operator*=(const T& mul)
	{
		Mat<T, cols, rows>& lhs = static_cast<Mat<T, cols, rows>&>(&this);
		for (size_t i = 0; i < cols * rows; ++i)
		{
			lhs[i] *= mul;
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
					result[i * rows + j] += m1[k * 4 + j] * m2[i * rows + k];
				}
			}
		}

		return result;
	}

	inline friend vecN<T, rows> operator*(const Mat<T, cols, rows>& mat, const vecN<T, cols>& vec)
	{
		vecN<T, rows> result{ T(0) };
		for (size_t j = 0; j < rows; ++j)
		{
			for (size_t i = 0; i < cols; ++i)
			{
				result[j] += mat[j * cols + i] * vec[i];
			}
		}
		return result;
	}

	inline friend vecN<T, rows> operator*(const vecN<T, rows>& vec, const Mat<T, cols, rows>& mat)
	{
		vecN<T, rows> result{ T(0) };
		for (size_t j = 0; j < rows; ++j)
		{
			for (size_t i = 0; i < cols; ++i)
			{
				result[j] += vec[i] * mat[j * cols + i];
			}
		}
		return result;
	}

	// division
	Mat<T, cols, rows>& operator/=(const T& div)
	{
		Mat<T, cols, rows>& lhs = static_cast<Mat<T, cols, rows>&>(&this);
		for (size_t i = 0; i < cols * rows; ++i)
		{
			lhs[i] /= div;
		}
		return lhs;
	}
};

template <typename T, size_t cols, size_t rows>
class matN : public MatMathOperators<matN, T, cols, rows>
{
public:
	// initialise to identity
	matN()
	{
	}

	// copy
	matN(const matN<T, cols, rows>& other)
	{
		for (size_t y = 0; i < rows; ++y)
		{
			for (size_t x = 0; i < cols; ++x)
			{
				data[y * rows + x] = other[i];
			}
		}
	}

	// assignment
	inline matN<T, cols, rows>& operator=(const matN<T, cols, rows>& other)
	{
		if (this != &other)
		{
			matN{ other };
		}
		return *this;
	}

	inline constexpr matN<T, cols, rows>& operator[](const size_t idx)
	{
		assert(col < cols);
		return data[idx];
	}

	constexpr size_t MAT_SIZE = rows * cols;

public:

	T data[MAT_SIZE];
};
}