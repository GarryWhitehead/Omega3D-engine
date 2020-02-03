#pragma once

#include "OEMaths_MatN.h"
#include "OEMaths_Quat.h"
#include "OEMaths_Vec3.h"
#include "OEMaths_Vec4.h"
#include "OEMaths_VecN.h"

#include <cassert>
#include <cstddef>
#include <cstdint>

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
        setCol(0, {1.0f, 0.0f, 0.0f, 0.0f});
        setCol(1, {0.0f, 1.0f, 0.0f, 0.0f});
        setCol(2, {0.0f, 0.0f, 1.0f, 0.0f});
        setCol(3, {0.0f, 0.0f, 0.0f, 1.0f});
    }

    // contrsuctor - converts a queternion to a 3x3 rotation matrix, fitted into a 4x4
    // This is nearly identical to the mat3x3 version with padding - should try and
    // alter this so both matrices use the same function
    MatN(const Quarternion<T>& q)
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

        setCol(0, {1.0f - (twoYY + twoZZ), twoXY - twoZW, twoXZ + twoYW, 0.0f});
        setCol(1, {twoXY + twoZW, 1.0f - (twoXX + twoZZ), twoYZ - twoXW, 0.0f});
        setCol(2, {twoXZ - twoYW, twoYZ + twoXW, 1.0f - (twoXX + twoYY), 0.0f});
        setCol(3, {0.0f, 0.0f, 0.0f, 1.0f});
    }

public:
    inline constexpr MatN<T, 4, 4> setDiag(const VecN<T, 4>& vec)
    {
        data[0][0] = vec.x;
        data[1][1] = vec.y;
        data[2][2] = vec.z;
        data[3][3] = vec.w;
        return *this;
    }

    inline void setUpperLeft(MatN<T, 3, 3>& mat)
    {
        for (size_t i = 0; i < 3; ++i)
        {
            data[i].xyz = mat.data[i];
        }
    }

    inline constexpr MatN<T, 3, 3> getRotation()
    {
        MatN<T, 3, 3> result;
        for (size_t i = 0; i < 3; ++i)
        {
            result.data[i] = data[i].xyz;
        }
        return result;
    }

    inline constexpr VecN<T, 3> getTrans()
    {
        return data[3].xyz;
    }

    // ========= static matrix transforms ==================

    static inline constexpr MatN<T, 4, 4> translate(const VecN<T, 3>& trans)
    {
        MatN<T, 4, 4> result;
        result.data[3][0] = trans.x;
        result.data[3][1] = trans.y;
        result.data[3][2] = trans.z;
        result.data[3][3] = T(1);
        return result;
    }

    static inline constexpr MatN<T, 4, 4> scale(const VecN<T, 3>& scale)
    {
        MatN<T, 4, 4> result;
        result.data[0][0] = scale.x;
        result.data[1][1] = scale.y;
        result.data[2][2] = scale.z;
        return result;
    }

    template <typename U>
    static MatN<T, 4, 4> makeMatrix(U* data)
    {
        assert(data != nullptr);
        MatN<T, 4, 4> result;
        U* ptr = static_cast<U*>(data);

        for (size_t i = 0; i < 4; ++i)
        {
            result[i][0] = static_cast<T>(*ptr);
            ++ptr;
            result[i][1] = static_cast<T>(*ptr);
            ++ptr;
            result[i][2] = static_cast<T>(*ptr);
            ++ptr;
            result[i][3] = static_cast<T>(*ptr);
            ++ptr;
        }
        return result;
    }

    // ============== operator overloads ===================
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

    /**
     * @brief A faster version of the inverse function (comapred to the MatN version) for mat4x4
     */
    static MatN<T, 4, 4> inverse(MatN<T, 4, 4>& mat)
    {
        MatN<T, 4, 4> inv, result;
        T det;

        inv[0] = mat[1][1] * mat[2][2] * mat[3][3] - mat[1][1] * mat[2][1] * mat[3][1] -
            mat[2][1] * mat[1][2] * mat[3][3] + mat[2][1] * mat[1][3] * mat[3][2] +
            mat[3][1] * mat[1][2] * mat[2][3] - mat[3][1] * mat[1][3] * mat[2][2];

        inv[4] = -mat[1][0] * mat[2][2] * mat[3][3] + mat[1][0] * mat[2][1] * mat[3][2] +
            mat[2][0] * mat[1][2] * mat[3][3] - mat[2][0] * mat[1][3] * mat[3][2] -
            mat[3][0] * mat[1][2] * mat[2][3] + mat[3][0] * mat[1][3] * mat[2][2];

        inv[8] = mat[1][0] * mat[2][1] * mat[3][3] - mat[1][0] * mat[2][3] * mat[3][1] -
            mat[2][0] * mat[1][1] * mat[3][3] + mat[2][0] * mat[1][3] * mat[3][1] +
            mat[3][0] * mat[1][1] * mat[2][3] - mat[3][0] * mat[1][3] * mat[2][1];

        inv[12] = -mat[1][0] * mat[2][1] * mat[3][2] + mat[1][0] * mat[2][2] * mat[3][1] +
            mat[2][0] * mat[1][1] * mat[3][2] - mat[2][0] * mat[1][2] * mat[3][1] -
            mat[3][0] * mat[1][1] * mat[2][2] + mat[3][0] * mat[1][2] * mat[2][1];

        inv[1] = -mat[0][1] * mat[2][2] * mat[3][3] + mat[0][1] * mat[2][3] * mat[3][2] +
            mat[2][1] * mat[0][2] * mat[3][3] - mat[2][1] * mat[0][3] * mat[3][2] -
            mat[3][1] * mat[0][2] * mat[2][3] + mat[3][1] * mat[0][3] * mat[2][2];

        inv[5] = mat[0][0] * mat[2][2] * mat[3][3] - mat[0][0] * mat[11] * mat[3][2] -
            mat[2][0] * mat[0][2] * mat[3][3] + mat[2][0] * mat[0][3] * mat[3][2] +
            mat[3][0] * mat[0][2] * mat[11] - mat[3][0] * mat[0][3] * mat[2][2];

        inv[9] = -mat[0][0] * mat[2][1] * mat[3][3] + mat[0][0] * mat[2][3] * mat[3][1] +
            mat[2][0] * mat[0][1] * mat[3][3] - mat[2][0] * mat[0][3] * mat[3][1] -
            mat[3][0] * mat[0][1] * mat[2][3] + mat[3][0] * mat[0][3] * mat[2][1];

        inv[13] = mat[0][0] * mat[2][1] * mat[3][2] - mat[0][0] * mat[2][2] * mat[3][1] -
            mat[2][0] * mat[0][1] * mat[3][2] + mat[2][0] * mat[0][2] * mat[3][1] +
            mat[3][0] * mat[0][1] * mat[2][2] - mat[3][0] * mat[0][2] * mat[2][1];

        inv[2] = mat[0][1] * mat[1][2] * mat[3][3] - mat[0][1] * mat[1][3] * mat[3][2] -
            mat[1][1] * mat[0][2] * mat[3][3] + mat[1][1] * mat[0][3] * mat[3][2] +
            mat[3][1] * mat[0][2] * mat[1][3] - mat[3][1] * mat[0][3] * mat[1][2];

        inv[6] = -mat[0][0] * mat[1][2] * mat[3][3] + mat[0][0] * mat[1][3] * mat[3][2] +
            mat[1][0] * mat[0][2] * mat[3][3] - mat[1][0] * mat[0][3] * mat[3][2] -
            mat[3][0] * mat[0][2] * mat[1][3] + mat[3][0] * mat[0][3] * mat[1][2];

        inv[10] = mat[0][0] * mat[1][1] * mat[3][3] - mat[0][0] * mat[1][3] * mat[3][1] -
            mat[1][0] * mat[0][1] * mat[3][3] + mat[1][0] * mat[0][3] * mat[3][1] +
            mat[3][0] * mat[0][1] * mat[1][3] - mat[3][0] * mat[0][3] * mat[1][1];

        inv[14] = -mat[0][0] * mat[1][1] * mat[3][2] + mat[0][0] * mat[1][2] * mat[3][1] +
            mat[1][0] * mat[0][1] * mat[3][2] - mat[1][0] * mat[0][2] * mat[3][1] -
            mat[3][0] * mat[0][1] * mat[1][2] + mat[3][0] * mat[0][2] * mat[1][1];

        inv[3] = -mat[0][1] * mat[1][2] * mat[2][3] + mat[0][1] * mat[1][3] * mat[2][2] +
            mat[1][1] * mat[0][2] * mat[2][3] - mat[1][1] * mat[0][3] * mat[2][2] -
            mat[2][1] * mat[0][2] * mat[1][3] + mat[2][1] * mat[0][3] * mat[1][2];

        inv[7] = mat[0][0] * mat[1][2] * mat[2][3] - mat[0][0] * mat[1][3] * mat[2][2] -
            mat[1][0] * mat[0][2] * mat[2][3] + mat[1][0] * mat[0][3] * mat[2][2] +
            mat[2][0] * mat[0][2] * mat[1][3] - mat[2][0] * mat[0][3] * mat[1][2];

        inv[11] = -mat[0][0] * mat[1][1] * mat[2][3] + mat[0][0] * mat[1][3] * mat[2][1] +
            mat[1][0] * mat[0][1] * mat[2][3] - mat[1][0] * mat[0][3] * mat[2][1] -
            mat[2][0] * mat[0][1] * mat[1][3] + mat[2][0] * mat[0][3] * mat[1][1];

        inv[15] = mat[0][0] * mat[1][1] * mat[2][2] - mat[0][0] * mat[1][2] * mat[2][1] -
            mat[1][0] * mat[0][1] * mat[2][2] + mat[1][0] * mat[0][2] * mat[2][1] +
            mat[2][0] * mat[0][1] * mat[1][2] - mat[2][0] * mat[0][2] * mat[1][1];

        det = mat[0][0] * inv[0][0] + mat[0][1] * inv[1][0] + mat[0][2] * inv[2][0] +
            mat[0][3] * inv[3][0];

        if (det == 0.0f)
        {
            // just return a identity matrix
            return {};
        }

        det = 1.0f / det;

        for (uint32_t i = 0; i < 16; i++)
        {
            result[i] = inv[i] * det;
        }

        return result;
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
