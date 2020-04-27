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

#include "Frustum.h"

#include "Types/AABox.h"

namespace OmegaEngine
{

void Frustum::projection(OEMaths::mat4f viewProj)
{
    enum Face
    {
        Left,
        Right,
        Top,
        Bottom,
        Back,
        Front
    };

    planes[Left] = viewProj[3] - viewProj[0];
    planes[Right] = viewProj[3] + viewProj[0];
    planes[Top] = viewProj[3] + viewProj[1];
    planes[Bottom] = viewProj[3] - viewProj[1];
    planes[Front] = viewProj[3] - viewProj[2];
    planes[Back] = viewProj[3] + viewProj[2];

    for (uint8_t i = 0; i < 6; ++i)
    {
        float len = OEMaths::length(planes[i]);
        planes[i] /= len;
    }
}

bool Frustum::checkBoxPlaneIntersect(AABBox& box)
{
    for (size_t i = 0; i < 6; ++i)
    {
        // get the sign of the plane projection
        const uint32_t sx = (*((uint32_t*) &(planes[i].x)) >> 31);
        const uint32_t sy = (*((uint32_t*) &(planes[i].y)) >> 31);
        const uint32_t sz = (*((uint32_t*) &(planes[i].z)) >> 31);

        // check for plane intersect
        const float min = planes[i].x * box.extents[sx].x + planes[i].y * box.extents[sy].y +
            planes[i].z * box.extents[sz].z + planes[i].w;
        if (min > 0.0f)
        {
            // non-negative so the box is outside the plane
            return false;
        }

        const float max = planes[i].x * box.extents[sx ^ 1].x +
            planes[i].y * box.extents[sy ^ 1].y + planes[i].z * box.extents[sz ^ 1].z + planes[i].w;
        if (max > 0.0f)
        {
            // this is a intersect - in some cases it might be worth returning an enum instead here
            return true;
        }
    }
    return true;
}

bool Frustum::checkSphereIntersect(OEMaths::vec3f& center, float radius)
{
    for (size_t i = 0; i < 6; ++i)
    {
        const float dot =
            planes[i].x * center.x + planes[i].y * center.y * planes[i].z + center.z + planes[i].w;
        if (dot <= -radius)
        {
            return false;
        }
    }
    return true;
}

} // namespace OmegaEngine
