#pragma once

#include "OEMaths_MatN.h"
#include "OEMaths_VecN.h"
#include "OEMaths_Vec4.h"
#include "OEMaths_Vec3.h"
#include "OEMaths_Quat.h"

#include <cassert>
#include <cstdint>
#include <cstddef>

namespace OEMaths
{

template <typename T>
class MatN<T, 4, 4> : public MatMathOperators<MatN, T, 4, 4>
{
public:
    
    static constexpr size_t NUM_ROWS = 4;
    static constexpr size_t NUM_COLS = 4;
    static constexpr size_t MAT_SIZE = 16;
    
	inline void setCol(size_t col, const VecN<T, 4>& vec)
	{
        assert(col < 4);
        data[col] = vec;
	}

	// init as identity matrix
	MatN()
	{ 
		setCol(0, { 1.0f, 0.0f, 0.0f, 0.0f });
		setCol(1, { 0.0f, 1.0f, 0.0f, 0.0f });
		setCol(2, { 0.0f, 0.0f, 1.0f, 0.0f });
		setCol(3, { 0.0f, 0.0f, 0.0f, 1.0f });
	}

	// contrsuctor - converts a queternion to a 3x3 rotation matrix, fitted into a 4x4
    // This is nearly identical to the mat3x3 version with padding - should try and
    // alter this so both matrices use the same function
	MatN(Quarternion<T>& q)
    {
        float twoX = 2.0f * q.x;
        float twoY = 2.0f * q.y;
        float twoZ = 2.0f * q.z;

        float twoXX = twoX * q.x;
        float twoXY = twoX * q.y;
        float twoXZ = twoX * q.z;
        float twoXW = twoX * q.w;

        float twoYY = twoY * q.y;
        float twoYZ = twoY * q.z;
        float twoYW = twoY * q.w;

        float twoZZ = twoZ * q.z;
        float twoZW = twoZ * q.w;

        setCol(0, { 1.0f - (twoYY + twoZZ), twoXY - twoZW, twoXZ + twoYW, 0.0f });
        setCol(1, { twoXY + twoZW, 1.0f - (twoXX + twoZZ), twoYZ - twoXW, 0.0f });
        setCol(2, { twoXZ - twoYW, twoYZ + twoXW, 1.0f - (twoXX + twoYY), 0.0f });
        setCol(3, { 0.0f, 0.0f, 0.0f, 1.0f });
    }

public:

	// ========= matrix transforms ==================
    
    inline constexpr MatN<T, 4, 4>& setDiag(const VecN<T, 4>& vec)
    {
        data[0][0] = vec.x;
        data[1][1] = vec.y;
        data[2][2] = vec.z;
        data[3][3] = vec.w;
        return *this;
    }
    
    inline constexpr MatN<T, 4, 4>& translate(const VecN<T, 3>& trans)
	{
		data[3][0] = trans.x;
		data[3][1] = trans.y;
		data[3][2] = trans.z;
		data[3][3]  = T(1);
		return *this;
	}

	inline constexpr MatN<T, 4, 4>& scale(const VecN<T, 3>& scale)
	{
        setDiag({scale, 1.0f});
		return *this;
	}
    
    inline void setUpperLeft(MatN<T, 3, 3>& mat)
    {
        for (size_t i = 0; i < 3; ++i)
        {
			data[i].xyz = mat.data[i];
        }
    }
    
    inline constexpr MatN<T, 3, 3> getUpperLeft()
    {
        MatN<T, 3 ,3> result;
        for (size_t i = 0; i < 3; ++i)
        {
            result.data[i] = data[i].xyz;
        }
        return result;
    }
    
	// operator overloads
	inline constexpr VecN<T, 4>& operator[](const size_t& idx)
	{
		assert(idx < NUM_COLS);
		return data[idx];
	}
    
    // operator overloads
    inline constexpr VecN<T, 4> operator[](const size_t& idx) const
    {
        assert(idx < NUM_COLS);
        return data[idx];
    }


public:
	
    /**
     * coloumn major - data can be accesed by [col][row] format.
     */
    VecN<T, NUM_ROWS> data[NUM_COLS];
    
};

using mat4f = MatN<float, 4, 4>;
using mat4d = MatN<double, 4, 4>;

} // namespace OEMaths
