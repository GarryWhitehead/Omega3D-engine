#include "OEMaths_Mat4.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "OEMaths/OEMaths_Vec3.h"
#include "OEMaths/OEMaths_Vec4.h"

#include <algorithm>
#include <assert.h>
#include <cmath>

namespace OEMaths
{
mat4f::mat4f(const float *mat_data)
{
	assert(data != nullptr);
	float *ptr = (float *)mat_data;

	for (uint8_t col = 0; col < 4; ++col)
	{
		for (uint8_t row = 0; row < 4; ++row)
		{
			this->data[col * 4 + row] = *ptr;
			++ptr;
		}
	}
}

mat4f::mat4f(const double *mat_data)
{
	assert(data != nullptr);
	double *ptr = (double *)mat_data;

	for (uint8_t col = 0; col < 4; ++col)
	{
		for (uint8_t row = 0; row < 4; ++row)
		{
			this->data[col * 4 + row] = (float)*ptr;
			++ptr;
		}
	}
}

mat4f::mat4f(quatf &q)
{
	float twoX = 2.0f * q.getX();
	float twoY = 2.0f * q.getY();
	float twoZ = 2.0f * q.getZ();

	float twoXX = twoX * q.getX();
	float twoXY = twoX * q.getY();
	float twoXZ = twoX * q.getZ();
	float twoXW = twoX * q.getW();

	float twoYY = twoY * q.getY();
	float twoYZ = twoY * q.getZ();
	float twoYW = twoY * q.getW();

	float twoZZ = twoZ * q.getZ();
	float twoZW = twoZ * q.getW();

	vec4f v0{ 1.0f - (twoYY + twoZZ), twoXY - twoZW, twoXZ + twoYW, 0.0f };
	vec4f v1{ twoXY + twoZW, 1.0f - (twoXX + twoZZ), twoYZ - twoXW, 0.0f };
	vec4f v2{ twoXZ - twoYW, twoYZ + twoXW, 1.0f - (twoXX + twoYY), 0.0f };
	vec4f v3{ 0.0f, 0.0f, 0.0f, 1.0f };

	setCol(0, v0);
	setCol(1, v1);
	setCol(2, v2);
	setCol(3, v3);
}

float &mat4f::operator()(const uint8_t &col, const uint8_t &row)
{
	// using col major
	assert(row < 4 && col < 4);
	return data[col * 4 + row];
}

mat4f &mat4f::operator()(const vec4f &vec, const uint8_t &col)
{
	assert(col < 4);
	data[col * 4] = vec.getX();
	data[col * 4 + 1] = vec.getY();
	data[col * 4 + 2] = vec.getZ();
	data[col * 4 + 3] = vec.getW();
	return *this;
}

mat4f &mat4f::operator/=(const float &div)
{
	const float invDiv = 1 / div;
	data[0] /= div;
	data[1] /= div;
	data[2] /= div;
	data[3] /= div;

	data[4] /= div;
	data[5] /= div;
	data[6] /= div;
	data[7] /= div;

	data[8] /= div;
	data[9] /= div;
	data[10] /= div;
	data[11] /= div;

	data[12] /= div;
	data[13] /= div;
	data[14] /= div;
	data[15] /= div;
	return *this;
}

float &mat4f::operator[](const uint32_t &index)
{
	return data[index];
}

void mat4f::setCol(const uint8_t col, vec4f &v)
{
	assert(col < 4);
	uint8_t row = 0;
	data[col * 4 + row] = v.getX();
	row++;
	data[col * 4 + row] = v.getY();
	row++;
	data[col * 4 + row] = v.getZ();
	row++;
	data[col * 4 + row] = v.getW();
}

vec4f operator*(const mat4f &mat, const vec4f &vec)
{
	vec4f result;
	result.x =
	    mat.data[0] * vec.x + mat.data[1] * vec.y + mat.data[2] * vec.z + mat.data[3] * vec.w;
	result.y =
	    mat.data[4] * vec.x + mat.data[5] * vec.y + mat.data[6] * vec.z + mat.data[7] * vec.w;
	result.z =
	    mat.data[8] * vec.x + mat.data[9] * vec.y + mat.data[10] * vec.z + mat.data[11] * vec.w;
	result.w =
	    mat.data[12] * vec.x + mat.data[13] * vec.y + mat.data[14] * vec.z + mat.data[15] * vec.w;
	return result;
}

vec4f operator*(const vec4f &vec, const mat4f &mat)
{
	vec4f result;
	result.x =
	    vec.x * mat.data[0] + vec.y * mat.data[1] + vec.z * mat.data[2] + vec.w * mat.data[3];
	result.y =
	    vec.x * mat.data[4] + vec.y * mat.data[5] + vec.z * mat.data[6] + vec.w * mat.data[7];
	result.z =
	    vec.x * mat.data[8] + vec.y * mat.data[9] + vec.z * mat.data[10] + vec.w * mat.data[11];
	result.w =
	    vec.x * mat.data[12] + vec.y * mat.data[13] + vec.z * mat.data[14] + vec.w * mat.data[15];
	return result;
}

mat4f operator*(const mat4f &m1, const mat4f &m2)
{
	mat4f result;

	for (uint8_t row = 0; row < 4; ++row)
	{

		for (uint8_t col = 0; col < 4; ++col)
		{
			result.data[col * 4 + row] = 0;

			for (uint8_t k = 0; k < 4; ++k)
			{
				result.data[col * 4 + row] += m1.data[k * 4 + row] * m2.data[col * 4 + k];
			}
		}
	}

	return result;
}

mat4f mat4f::translate(vec3f &trans)
{
	mat4f result;
	result[12] = trans.getX();
	result[13] = trans.getY();
	result[14] = trans.getX();
	result[15] = 1.0f;
	return result;
}

mat4f mat4f::scale(vec3f &scale)
{
	mat4f result;
	result[0] = scale.getX();
	result[5] = scale.getY();
	result[10] = scale.getZ();
	result[15] = 1.0f;
	return result;
}

mat4f mat4f::rotate(float theta, vec3f &axis)
{
	mat4f result;

	const float angleRad = radians(theta);

	vec3f axis_norm = axis / (angleRad == 0.0f ? 1.0f : angleRad); //avoid divide by zero
	const float xy = axis_norm.getX() * axis_norm.getY();
	const float yz = axis_norm.getY() * axis_norm.getZ();
	const float zx = axis_norm.getZ() * axis_norm.getX();

	const float cosTheta = std::cos(angleRad);
	const float sinTheta = std::sin(angleRad);

	result(0, 0) = cosTheta + axis_norm.getX() * axis_norm.getX() * (1.0f - cosTheta);
	result(0, 1) = xy * (1.0f - cosTheta) - axis_norm.getZ() * sinTheta;
	result(0, 2) = zx * (1.0f - cosTheta) + axis_norm.getY() * sinTheta;

	result(1, 0) = xy * (1.0f - cosTheta) + axis_norm.getZ() * sinTheta;
	result(1, 1) = cosTheta + axis_norm.getY() * axis_norm.getY() * (1.0f - cosTheta);
	result(1, 2) = yz * (1.0f - cosTheta) - axis_norm.getX() * sinTheta;

	result(2, 0) = zx * (1.0f - cosTheta) - axis_norm.getY() * sinTheta;
	result(2, 1) = yz * (1.0f - cosTheta) + axis_norm.getX() * sinTheta;
	result(2, 2) = cosTheta + axis_norm.getZ() * axis_norm.getZ() * (1.0f - cosTheta);

	return result;
}

mat4f mat4f::inverse()
{
	mat4f inv, result;
	float det;

	inv[0] = data[5] * data[10] * data[15] - data[5] * data[11] * data[14] -
	         data[9] * data[6] * data[15] + data[9] * data[7] * data[14] +
	         data[13] * data[6] * data[11] - data[13] * data[7] * data[10];

	inv[4] = -data[4] * data[10] * data[15] + data[4] * data[11] * data[14] +
	         data[8] * data[6] * data[15] - data[8] * data[7] * data[14] -
	         data[12] * data[6] * data[11] + data[12] * data[7] * data[10];

	inv[8] = data[4] * data[9] * data[15] - data[4] * data[11] * data[13] -
	         data[8] * data[5] * data[15] + data[8] * data[7] * data[13] +
	         data[12] * data[5] * data[11] - data[12] * data[7] * data[9];

	inv[12] = -data[4] * data[9] * data[14] + data[4] * data[10] * data[13] +
	          data[8] * data[5] * data[14] - data[8] * data[6] * data[13] -
	          data[12] * data[5] * data[10] + data[12] * data[6] * data[9];

	inv[1] = -data[1] * data[10] * data[15] + data[1] * data[11] * data[14] +
	         data[9] * data[2] * data[15] - data[9] * data[3] * data[14] -
	         data[13] * data[2] * data[11] + data[13] * data[3] * data[10];

	inv[5] = data[0] * data[10] * data[15] - data[0] * data[11] * data[14] -
	         data[8] * data[2] * data[15] + data[8] * data[3] * data[14] +
	         data[12] * data[2] * data[11] - data[12] * data[3] * data[10];

	inv[9] = -data[0] * data[9] * data[15] + data[0] * data[11] * data[13] +
	         data[8] * data[1] * data[15] - data[8] * data[3] * data[13] -
	         data[12] * data[1] * data[11] + data[12] * data[3] * data[9];

	inv[13] = data[0] * data[9] * data[14] - data[0] * data[10] * data[13] -
	          data[8] * data[1] * data[14] + data[8] * data[2] * data[13] +
	          data[12] * data[1] * data[10] - data[12] * data[2] * data[9];

	inv[2] = data[1] * data[6] * data[15] - data[1] * data[7] * data[14] -
	         data[5] * data[2] * data[15] + data[5] * data[3] * data[14] +
	         data[13] * data[2] * data[7] - data[13] * data[3] * data[6];

	inv[6] = -data[0] * data[6] * data[15] + data[0] * data[7] * data[14] +
	         data[4] * data[2] * data[15] - data[4] * data[3] * data[14] -
	         data[12] * data[2] * data[7] + data[12] * data[3] * data[6];

	inv[10] = data[0] * data[5] * data[15] - data[0] * data[7] * data[13] -
	          data[4] * data[1] * data[15] + data[4] * data[3] * data[13] +
	          data[12] * data[1] * data[7] - data[12] * data[3] * data[5];

	inv[14] = -data[0] * data[5] * data[14] + data[0] * data[6] * data[13] +
	          data[4] * data[1] * data[14] - data[4] * data[2] * data[13] -
	          data[12] * data[1] * data[6] + data[12] * data[2] * data[5];

	inv[3] = -data[1] * data[6] * data[11] + data[1] * data[7] * data[10] +
	         data[5] * data[2] * data[11] - data[5] * data[3] * data[10] -
	         data[9] * data[2] * data[7] + data[9] * data[3] * data[6];

	inv[7] = data[0] * data[6] * data[11] - data[0] * data[7] * data[10] -
	         data[4] * data[2] * data[11] + data[4] * data[3] * data[10] +
	         data[8] * data[2] * data[7] - data[8] * data[3] * data[6];

	inv[11] = -data[0] * data[5] * data[11] + data[0] * data[7] * data[9] +
	          data[4] * data[1] * data[11] - data[4] * data[3] * data[9] -
	          data[8] * data[1] * data[7] + data[8] * data[3] * data[5];

	inv[15] = data[0] * data[5] * data[10] - data[0] * data[6] * data[9] -
	          data[4] * data[1] * data[10] + data[4] * data[2] * data[9] +
	          data[8] * data[1] * data[6] - data[8] * data[2] * data[5];

	det = data[0] * inv[0] + data[1] * inv[4] + data[2] * inv[8] + data[3] * inv[12];

	if (det == 0.0f)
	{
		// just return a identity matrix
		return OEMaths::mat4f();
	}

	det = 1.0f / det;

	for (uint32_t i = 0; i < 16; i++)
	{
		result[i] = inv[i] * det;
	}

	return result;
}
} // namespace OEMaths