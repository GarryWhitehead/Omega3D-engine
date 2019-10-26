#pragma once

#include "OEMaths_MatN.h"
#include "OEMaths_Vec2.h"

#include <cstdint>

namespace OEMaths
{

template <typename T>
class MatN<T, 2, 2>
{
public:

    void setCol(size_t col, const VecN<T, 2>& vec)
    {
        assert(col < NUM_COLS);
        data[col] = vec;
    }

    // init as identity matrix
    MatN()
    {
        setCol(0, { 1.0f, 0.0f });
        setCol(1, { 0.0f, 1.0f });
    }

    T &operator[](const size_t &idx)
    {
        assert(idx < NUM_COLS);
        return data[idx];
    }

public:

    // ========= matrix transforms ==================

    MatN<T, 2, 2> setDiag(const VecN<T, 2>& vec)
    {
        data[0][0] = vec.x;
        data[1][1] = vec.y;
        return *this;
    }
    
    static constexpr size_t NUM_ROWS = 2;
    static constexpr size_t NUM_COLS = 2;
    static constexpr size_t MAT_SIZE = 4;

private:
    
    /**
     * coloumn major - data can be accesed by [col][row] format.
     */
    VecN<T, NUM_ROWS> data[NUM_COLS];
    
};

using mat2f = MatN<float, 2, 2>;
using mat2d = MatN<double, 2, 2>;

} // namespace OEMaths
