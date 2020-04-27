/* Copyright (c) 2018-2020 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "OEMaths/OEMaths.h"

#include <cmath>

namespace OmegaEngine
{
namespace Ibl
{

inline OEMaths::vec2f hammersley(uint64_t i, uint64_t N)
{
    uint64_t bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rad = float(bits) * 2.3283064365386963e-10f; // 0x100000000
    return OEMaths::vec2f(float(i) / float(N), rad);
}

inline OEMaths::vec3f importanceSampleGGX(OEMaths::vec2f Xi, OEMaths::vec3f N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0f * M_PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // spherical to cartesian coordinates
    OEMaths::vec3f H = OEMaths::vec3f(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // from tangent-space vector to world-space sample vector
    OEMaths::vec3f up =
        abs(N.z) < 0.999 ? OEMaths::vec3f(0.0f, 0.0f, 1.0f) : OEMaths::vec3f(1.0f, 0.0f, 0.0f);
    OEMaths::vec3f tanX = OEMaths::normalise(OEMaths::vec3f::cross(up, N));
    OEMaths::vec3f tanY = OEMaths::normalise(OEMaths::vec3f::cross(N, tanX));
    return OEMaths::normalise(tanX * H.x + tanY * H.y + N * H.z);
}

inline float geometryShlickGGX(float NdotV, float NdotL, float roughness)
{
    float k = (roughness * roughness) / 2.0f;
    float GV = NdotV / (NdotV * (1.0f - k) + k);
    float GL = NdotL / (NdotL * (1.0f - k) + k);
    return GL * GV;
}

inline float distributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;

    return (a2) / (M_PI * denom * denom);
}

} // namespace Ibl
} // namespace OmegaEngine
