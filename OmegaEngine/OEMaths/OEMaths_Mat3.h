#pragma once

#include "OEMaths_MatN.h"
#include "OEMaths_VecN.h"
#include "OEMaths_Vec3.h"
#include "OEMaths_Quat.h"

#include <cstdint>

namespace OEMaths
{

template <typename T>
class MatN<T, 3, 3> : public MatMathOperators<MatN, T,3, 3>
{
public:

    void setCol(size_t col, const VecN<T, 3>& vec)
    {
        assert(col < NUM_COLS);
        data[col] = vec;
    }

    // init as identity matrix
    MatN()
    {
        setCol(0, { 1.0f, 0.0f, 0.0f });
        setCol(1, { 0.0f, 1.0f, 0.0f });
        setCol(2, { 0.0f, 0.0f, 1.0f });
    }

    // contrsuctor - converts a quaternion to a 3x3 matrix
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

        setCol(0, { 1.0f - (twoYY + twoZZ), twoXY - twoZW, twoXZ + twoYW });
        setCol(1, { twoXY + twoZW, 1.0f - (twoXX + twoZZ), twoYZ - twoXW });
        setCol(2, { twoXZ - twoYW, twoYZ + twoXW, 1.0f - (twoXX + twoYY) });
    }

public:

	// ========= operator overloads =========================

	VecN<T, 3>& operator[](const size_t& idx) const
	{
		assert(idx < NUM_COLS);
		return data[idx];
	}


    // ========= matrix transforms ==================

    MatN<T, 3, 3> setDiag(const VecN<T, 3>& vec)
    {
        data[0][0] = vec.x;
        data[1][1] = vec.y;
        data[2][2] = vec.z;
        return *this;
    }
    
    MatN<T, 3, 3> translate(const VecN<T, 3>& trans)
    {
        data[2][0] = trans.x;
        data[2][1] = trans.y;
        data[2][2] = trans.z;
        return *this;
    }

    MatN<T, 3, 3> scale(const VecN<T, 3>& scale)
    {
        setDiag(scale);
        return *this;
    }

    static constexpr size_t NUM_ROWS = 3;
    static constexpr size_t NUM_COLS = 3;
    static constexpr size_t MAT_SIZE = 9;

public:
    
    /**
     * coloumn major - data can be accesed by [col][row] format.
     */
    VecN<T, NUM_ROWS> data[NUM_COLS];
    
};

using mat3f = MatN<float, 3, 3>;
using mat3d = MatN<double, 3, 3>;


} // namespace OEMaths
