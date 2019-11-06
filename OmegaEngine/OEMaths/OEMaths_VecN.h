#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cassert>

namespace OEMaths
{
template <template <typename T, size_t size> typename Vec, typename T, size_t size>
class MathOperators
{
public:
	// addition
	friend inline Vec<T, size>& operator+(const Vec<T, size>& vec1, const Vec<T, size>& vec2)
	{
		Vec<T, size> result;
		for (size_t i = 0; i < size; ++i)
		{
			result[i] = vec1[i] + vec2[i];
		}
		return result;
	}

	inline Vec<T, size>& operator+=(const Vec<T, size>& other)
	{
		Vec<T, size>& lhs = static_cast<Vec<T, size>&>(&this);
		for (size_t i = 0; i < size; ++i)
		{
			lhs[i] += other[i];
		}
		return lhs;
	}

	// subtraction
	friend inline Vec<T, size>& operator-(const Vec<T, size>& vec1, const Vec<T, size>& vec2)
	{
		Vec<T, size> result;
		for (size_t i = 0; i < size; ++i)
		{
			result[i] = vec1[i] - vec2[i];
		}
		return result;
	}

	inline Vec<T, size>& operator-=(const Vec<T, size>& other)
	{
		Vec<T, size>& lhs = static_cast<Vec<T, size>&>(&this);
		for (size_t i = 0; i < size; ++i)
		{
			lhs[i] -= other[i];
		}
		return lhs;
	}


	// multiplication
	inline friend Vec<T, size>& operator*(const Vec<T, size>& vec, const T& value)
	{
		Vec<T, size> result;
		for (size_t i = 0; i < size; ++i)
		{
			result[i] = vec[i] * value;
		}
		return result;
	}

	inline friend Vec<T, size>& operator*(const T& value, const Vec<T, size>& vec)
	{
		Vec<T, size> result;
		for (size_t i = 0; i < size; ++i)
		{
			result[i] = value * vec[i];
		}
		return result;
	}

	inline friend Vec<T, size>& operator*(const Vec<T, size>& vec1, const Vec<T, size>& vec2)
	{
		Vec<T, size> result;
		for (size_t i = 0; i < size; ++i)
		{
			result[i] = vec1[i] * vec2[i];
		}
		return result;
	}

	inline Vec<T, size>& operator*=(const Vec<T, size>& other)
	{
		Vec<T, size>& lhs = static_cast<Vec<T, size>&>(&this);
		for (size_t i = 0; i < size; ++i)
		{
			lhs[i] *= other[i];
		}
		return lhs;
	}

	// division
	inline friend Vec<T, size>& operator/(const Vec<T, size>& vec, const T& value)
	{
		Vec<T, size> result;
		for (size_t i = 0; i < size; ++i)
		{
			result[i] = vec[i] / value;
		}
		return result;
	}

	inline friend Vec<T, size>& operator/(const T& value, const Vec<T, size>& vec)
	{
		Vec<T, size> result;
		for (size_t i = 0; i < size; ++i)
		{
			result[i] = value / vec[i];
		}
		return result;
	}

	inline friend Vec<T, size>& operator/(const Vec<T, size>& vec1, const Vec<T, size>& vec2)
	{
		Vec<T, size> result;
		for (size_t i = 0; i < size; ++i)
		{
			result[i] = vec1[i] / vec2[i];
		}
		return result;
	}

	inline Vec<T, size>& operator/=(const Vec<T, size>& other)
	{
		Vec<T, size>& lhs = static_cast<Vec<T, size>&>(&this);
		for (size_t i = 0; i < size; ++i)
		{
			lhs[i] /= other[i];
		}
		return lhs;
	}
	
};

template <typename T, size_t size>
class VecN : public MathOperators<VecN, T, size>
{
public:
	VecN() {}

	// copy
	VecN(const VecN<T, size>& other)
	{
		for (size_t i = 0; i < size; ++i)
		{
			data[i] = other.data[i];
		}
	}

	// assignment
	VecN<T, size>& operator=(const VecN<T, size>& other)
	{
		if (this != &other)
		{
			for (size_t i = 0; i < size; ++i)
			{
				data[i] = other.data[i];
			}
		}
		return *this;
	}

	VecN<T, size>& operator[](const size_t idx)
	{
		assert(idx < size);
		return data[idx];
	}

public:

	T data[size];
};

// =========== helpers =====================

template <typename T, size_t size, typename U>
VecN<T, size> makeVector(U* data)
{
	assert(data != nullptr);
	U* ptr = static_cast<U*>(data);

	VecN<T, size> result;
	for (size_t i = 0; i < size; ++i)
	{
		result[i] = (T)*ptr;
		++ptr;
	}
	return result;
}

// =========== vector transforms ====================

template <typename T, size_t size>
inline constexpr T length(VecN<T, size>& vec)
{
	T result;
	for (size_t i = 0; i < size; ++i)
	{
		result += vec.data[i] * vec.data[i];
	}
	return std::sqrt(result);
}

template <typename T, size_t size>
inline constexpr VecN<T, size>& normalise(VecN<T, size>& vec)
{
	VecN<T, size> result;

	T len = length(vec);

	for (size_t i = 0; i < size; ++i)
	{
		result.data[i] = vec.data[i] / len;
	}
	return result;
}

template <typename T, size_t size>
inline constexpr T dot(VecN<T, size>& vec1, VecN<T, size>& vec2)
{
	T result;
	for (size_t i = 0; i < size; ++i)
	{
		result += vec1.data[i] * vec2.data[i];
	}
	return result;
}

template <typename T, size_t size>
inline constexpr VecN<T, size>& mix(VecN<T, size>& vec1, VecN<T, size>& vec2, T u)
{
	VecN<T, size> result;
	for (size_t i = 0; i < size; ++i)
	{
		result.data[i] = vec1.data[i] * (1.0f - u) + vec2.data[i] * u;
	}
	return result;
}

}    // namespace OEMaths
