#pragma once

#include <cstdint>

namespace OEMaths
{
    class vec2f;

    class mat2f
    {
    public:

        mat2f()
        {
            data[0] = 1.0f;
            data[3] = 1.0f;
        }

        float& operator()(const uint8_t& col, const uint8_t& row);
        mat2f& operator()(vec2f& vec, const uint8_t& col);

    private:

        float data[4];
	};
}