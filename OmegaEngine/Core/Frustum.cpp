#include "Frustum.h"

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

bool Frustum::checkBoxIntersect(OEMaths::vec4f& box)
{
	bool result = false;
	for (size_t i = 0; i < 6; ++i)
	{
		const float dot = planes[i].x * box.x + planes[i].y * box.y * planes[i].z + box.z + planes[i].w - box.w;
		result &= dot < 0.0f ? true : false;
	}
	return result;
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