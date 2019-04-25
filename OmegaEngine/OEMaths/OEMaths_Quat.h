#pragma once

namespace OEMaths
{

	class quat
	{
	public:

		quatf() :
			x(0.0f),
			y(0.0f),
			z(0.0f),
			w(0.0f)
		{}

		quatf(float n) :
			x(n),
			y(n),
			z(n),
			w(n)
		{}

		quatf(float in_x, float in_y, float in_z, float in_w) :
			x(in_x),
			y(in_y),
			z(in_z),
			w(in_w)
		{}

		float x;
		float y;
		float z;
		float w;
	};


	// conversion functions
	quatf convert_quatf(void* data)
	{
		assert(data != nullptr);
		float* ptr = (float*)data;
		quatf qt;
		qt.x = *ptr;
		++ptr;
		qt.y = *ptr;
		++ptr;
		qt.z = *ptr;
		++ptr;
		qt.w = *ptr;
		return qt;
	}

	// conversion
	inline mat4f quat_to_mat4(quatf q)
	{
		mat4f mat;

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

		vec4f v0{ 1.0f - (twoYY + twoZZ), twoXY - twoZW, twoXZ + twoYW, 0.0f };
		vec4f v1{ twoXY + twoZW, 1.0f - (twoXX + twoZZ), twoYZ - twoXW, 0.0f };
		vec4f v2{ twoXZ - twoYW, twoYZ + twoXW, 1.0f - (twoXX + twoYY), 0.0f };
		vec4f v3{ 0.0f, 0.0f, 0.0f, 1.0f };

		mat(v0, 0);
		mat(v1, 1);
		mat(v2, 2);
		mat(v3, 3);
		return mat;
	}

	inline float length_quat(quatf& q)
	{
		return std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
	}

	inline quatf normalise_quat(quatf& q)
	{
		quatf result;
		float length = length_quat(q);

		result.x = q.x / length;
		result.y = q.y / length;
		result.z = q.z / length;
		result.w = q.w / length;
		return result;
	}

	inline quatf linear_mix_quat(quatf& q1, quatf& q2, float u)
	{
		quatf result;
		result.x = q1.x * (1.0f - u) + q2.x * u;
		result.y = q1.y * (1.0f - u) + q2.y * u;
		result.z = q1.z * (1.0f - u) + q2.z * u;
		result.w = q1.w * (1.0f - u) + q2.w * u;
		return result;
	}

	inline quatf cubic_mix_quat(quatf& q1, quatf& q2, quatf& q3, quatf& q4, float u)
	{
		quatf retVec;
		float a0, a1, a2, a3, u_sqr;
		u_sqr = u * u;

		// x
		a0 = -0.5 * q1.x + 1.5 * q2.x - 1.5 * q3.x + 0.5 * q4.x;
		a1 = q1.x - 2.5 * q2.x + 2.0 * q3.x - 0.5 * q4.x;
		a2 = -0.5 * q1.x + 0.5 * q3.x;
		a3 = q2.x;
		retVec.x = a0 * u * u_sqr + a1 * u_sqr + a2 * u + a3;

		// y
		a0 = -0.5 * q1.y + 1.5 * q2.y - 1.5 * q3.y + 0.5 * q4.y;
		a1 = q1.y - 2.5 * q2.y + 2.0 * q3.y - 0.5 * q4.y;
		a2 = -0.5 * q1.y + 0.5 * q3.y;
		a3 = q2.y;
		retVec.y = a0 * u * u_sqr + a1 * u_sqr + a2 * u + a3;

		// z
		a0 = -0.5 * q1.z + 1.5 * q2.z - 1.5 * q3.z + 0.5 * q4.z;
		a1 = q1.z - 2.5 * q2.z + 2.0 * q3.z - 0.5 * q4.z;
		a2 = -0.5 * q1.z + 0.5 * q3.z;
		a3 = q2.z;
		retVec.z = a0 * u * u_sqr + a1 * u_sqr + a2 * u + a3;

		// w
		a0 = -0.5 * q1.w + 1.5 * q2.w - 1.5 * q3.w + 0.5 * q4.w;
		a1 = q1.w - 2.5 * q2.w + 2.0 * q3.w - 0.5 * q4.w;
		a2 = -0.5 * q1.w + 0.5 * q3.w;
		a3 = q2.w;
		retVec.w = a0 * u * u_sqr + a1 * u_sqr + a2 * u + a3;
		return retVec;
	}


}