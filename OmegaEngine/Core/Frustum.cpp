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

bool Frustum::checkBoxPlaneIntersect(AABox& box)
{
	for (size_t i = 0; i < 6; ++i)
	{
		// get the sign of the plane projection
		const uint32_t sx = (*((uint32_t*)&(planes[i].x)) >> 31);
		const uint32_t sy = (*((uint32_t*)&(planes[i].y)) >> 31);
		const uint32_t sz = (*((uint32_t*)&(planes[i].z)) >> 31);

		// check for plane intersect
		const float min = planes[i].x * box.extents[sx].x + planes[i].y * box.extents[sy].y +
		                  planes[i].z * box.extents[sz].z + planes[i].w;
		if (min > 0.0f)
		{
			// non-negative so the box is outside the plane
			return false;
		}

		const float max = planes[i].x * box.extents[sx ^ 1].x + planes[i].y * box.extents[sy ^ 1].y +
		                  planes[i].z * box.extents[sz ^ 1].z + planes[i].w;
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
		const float dot = planes[i].x * center.x + planes[i].y * center.y * planes[i].z + center.z + planes[i].w;
		if (dot <= -radius)
		{
			return false;
		}
		return true;
	}
}

}    // namespace OmegaEngine
