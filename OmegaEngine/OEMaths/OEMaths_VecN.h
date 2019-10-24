#pragma once

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cmath>

namespace OEMaths
{

template <typename T, size_t size>
class vecN
{
public:
    
    vecN() = default;
    
    // copy
    vecN(const vecN<T, size> &other)
    {
        for (size_t i = 0; i < other.size(); ++i)
        {
            data[i] = other.data[i];
        }
    }
    
    // assignment
    vecN<T, size>& operator=(const vecN<T, size>& other)
    {
        if (this != &other)
        {
            for (size_t i = 0; i < other.size(); ++i)
            {
                data[i] = other.data[i];
            }
        }
        return *this;
    }
    
    // addition
    inline vecN<T, size>& operator+(const vecN<T, size> &other)
    {
        vecN<T, size> result;
        for (size_t i = 0; i < other.size(); ++i)
        {
            result[i] = data[i] + other.data[i];
        }
        return result;
    }
    
    inline vecN<T, size> &operator+=(const vecN<T, size> &other)
    {
        for (size_t i = 0; i < other.size(); ++i)
        {
            data[i] += other.data[i];
        }
        return *this;
    }
    
    // subtraction
    inline vecN<T, size>& operator-(const vecN<T, size> &other)
    {
        vecN<T, size> result;
        for (size_t i = 0; i < other.size(); ++i)
        {
            result[i] = data[i] - other.data[i];
        }
        return result;
    }
    
    inline vecN<T, size> &operator-=(const vecN<T, size> &other)
    {
        for (size_t i = 0; i < other.size(); ++i)
        {
            data[i] -= other.data[i];
        }
        return *this;
    }
    
    
    // multiplication
    inline vecN<T, size>& operator*(const vecN<T, size> &other)
    {
        vecN<T, size> result;
        for (size_t i = 0; i < other.size(); ++i)
        {
            result.data[i] = data[i] * other.data[i];
        }
        return result;
    }
    
    inline vecN<T, size> &operator*=(const vecN<T, size> &other)
    {
        for (size_t i = 0; i < other.size(); ++i)
        {
            data[i] *= other.data[i];
        }
        return *this;
    }
    
    // division
    inline vecN<T, size>& operator/(const vecN<T, size> &other)
    {
        vecN<T, size> result;
        for (size_t i = 0; i < other.size(); ++i)
        {
            result.data[i] = data[i] / other.data[i];
        }
        return result;
    }
    
    inline vecN<T, size> &operator/=(const vecN<T, size> &other)
    {
        for (size_t i = 0; i < other.size(); ++i)
        {
            data[i] /= other.data[i];
        }
        return *this;
    }
    
public:
    
    // linear algebra functions
    T length()
    {
        vecN<T, size> result;
        for (size_t i = 0; i < size; ++i)
        {
            result.data[i] += data[i] * data[i];
        }
        return std::sqrt(result);
    }
    
    vecN<T, size>& normalise()
    {
        T len = length();
        
        for (size_t i = 0; i < size; ++i)
        {
            data[i] / len;
        }
        return *this;
    }
    
    vecN<T, size> cross(vecN<T, size> &v1)
    {
        vecN<T, size> result;
        result.x = this->y * v1.getZ() - this->z * v1.getY();
        result.y = this->z * v1.getX() - this->x * v1.getZ();
        result.z = this->x * v1.getY() - this->y * v1.getX();
        for (size_t i = 0; i < other.size(); ++i)
        {
            result.data[i] = data[i] * v1.data[i] -
        }
        
    }
    
    T dot(vecN<T, size> &v)
    {
        T result;
        for (size_t i = 0; i < size; ++i)
        {
            result += data[i] * v.data[i];
        }
        return result;
    }
    
    vec3f mix(vec3f &v1, float u);
    
   
private:
    
    inline friend vecN<T, size>& operator*(const vecN<T, size> &vec, const T& value)
    {
        vecN<T, size> result;
        for (size_t i = 0; i < size; ++i)
        {
            result.data[i] = vec.data[i] * value;
        }
        return result;
    }
    
    inline friend vecN<T, size>& operator*(const T& value, const vecN<T, size> &vec)
    {
        vecN<T, size> result;
        for (size_t i = 0; i < size; ++i)
        {
            result.data[i] = value * vec.data[i];
        }
        return result;
    }
    
    inline friend vecN<T, size>& operator*(const vecN<T, size> &vec1, const vecN<T, size> &vec2)
    {
        vecN<T, size> result;
        for (size_t i = 0; i < size; ++i)
        {
            result.data[i] = vec1.data[i] * vec2.data[i];
        }
        return result;
    }
    
public:
    
    static const size_t vecSize = size;
    
    T data[size];
};


}
