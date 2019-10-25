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
class vecN : public MathOperators<vecN, T, size>
{
public:
	vecN() {}

	// copy
	vecN(const vecN<T, size>& other)
	{
		for (size_t i = 0; i < size; ++i)
		{
			data[i] = other.data[i];
		}
	}

	// assignment
	vecN<T, size>& operator=(const vecN<T, size>& other)
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

	vecN<T, size>& operator[](const size_t idx)
	{
		assert(idx < size);
		return data[idx];
	}

public:

	T data[size];
};

// =========== helpers =====================

template <typename T, size_t size, typename U>
vecN<T, size> make_vec(U* data)
{
	assert(data != nullptr);
	double* ptr = static_cast<U*>(data);

	vecN<T, size> result;
	for (size_t i = 0; i < size; ++i)
	{
		result.data[i] = (T)*ptr;
		++ptr;
	}
	return result;
}

// =========== vector transforms ====================

template <typename T, size_t size>
inline constexpr T length(vecN<T, size>& vec)
{
	T result;
	for (size_t i = 0; i < size; ++i)
	{
		result += vec.data[i] * vec.data[i];
	}
	return std::sqrt(result);
}

template <typename T, size_t size>
inline constexpr vecN<T, size>& normalise(vecN<T, size>& vec)
{
	vecN<T, size> result;

	T len = length(vec);

	for (size_t i = 0; i < size; ++i)
	{
		result.data[i] = vec.data[i] / len;
	}
	return result;
}

template <typename T, size_t size>
inline constexpr vecN<T, size> cross(vecN<T, size>& vec)
{
	vecN<T, size> result;
	result.x = this->y * v1.getZ() - this->z * v1.getY();
	result.y = this->z * v1.getX() - this->x * v1.getZ();
	result.z = this->x * v1.getY() - this->y * v1.getX();
}

template <typename T, size_t size>
inline constexpr T dot(vecN<T, size>& vec1, vecN<T, size>& vec2)
{
	T result;
	for (size_t i = 0; i < size; ++i)
	{
		result += vec1.data[i] * vec2.data[i];
	}
	return result;
}

template <typename T, size_t size>
inline constexpr vecN<T, size>& mix(vecN<T, size>& vec1, vecN<T, size>& vec2, T u)
{
	vecN<T, size> result;
	for (size_t i = 0; i < size; ++i)
	{
		result.data[i] = vec1.data[i] * (1.0f - u) + vec2.data[i] * u;
	}
	return result;
}

}    // namespace OEMaths
