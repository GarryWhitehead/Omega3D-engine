#pragma once

#include <OEMaths_MatN.h>
#include <OEMaths_Vec4.h>

#include <cassert>
#include <cstdint>
#include <cstddef>

namespace OEMaths
{
class vec4f;
class vec3f;
class quatf;

template <typename T>
class matN<T, 4, 4>
{
public:

	void setCol(size_t col, const vecN<T, 4>& vec)
	{
		data[col * 4] = vec[0];
		data[col * 4 + 1] = vec[1];
		data[col * 4 + 2] = vec[2];
		data[col * 4 + 3] = vec[3];
	}

	// init as identity matrix
	matN()
	{ 
		setCol(0, { 1.0f, 0.0f, 0.0f, 0.0f });
		setCol(1, { 0.0f, 1.0f, 0.0f, 0.0f });
		setCol(2, { 0.0f, 0.0f, 1.0f, 0.0f });
		setCol(3, { 0.0f, 0.0f, 0.0f, 1.0f });
	}

	// contrsuctor
	matN(quatf &q);


	T &operator[](const size_t &idx)
	{
		assert(idx < MAT_SIZE);
		return data[idx];
	}

public:

	// ========= matrix transforms ==================

	matN<T, 4, 4> translate(const vecN<T, 3>& trans)
	{
		matN<T, 4, 4> result;
		result[12] = trans.x;
		result[13] = trans.y;
		result[14] = trans.z;
		result[15] = T(1);
		return result;
	}

	matN<T, 4, 4> scale(const vecN<T, 3>& scale)
	{
		matN<T, 4, 4> result;
		result[0] = scale.x;
		result[5] = scale.y;
		result[10] = scale.z;
		result[15] = T(1);
		return result;
	}


	static constexpr size_t NUM_ROWS = 4;
	static constexpr size_t NUM_COLS = 4;
	static constexpr size_t MAT_SIZE = 16;

private:
	
	union
	{
		T data[MAT_SIZE];
	
		struct
		{
			vecN<T, 4> col[NUM_COLS];
		};
	};
};


} // namespace OEMaths