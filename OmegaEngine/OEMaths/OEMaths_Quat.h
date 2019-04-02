#pragma once

namespace OEMaths
{

	template <typename T>
	class quat
	{
	public:

		quat() :
			x(static_cast<T>(0)),
			y(static_cast<T>(0)),
			z(static_cast<T>(0)),
			w(static_cast<T>(0))
		{}

		quat(T n) :
			x(n),
			y(n),
			z(n),
			w(n)
		{}

		quat(T in_x, T in_y, T in_z, T in_w) :
			x(in_x),
			y(in_y),
			z(in_z),
			w(in_w)
		{}

		T x;
		T y;
		T z;
		T w;
	};

	using quatf = quat<float>;
	using quatd = quat<double>;


	// conversion functions
	template <typename T>
	quatf convert_quatf(void* data)
	{
		assert(data != nullptr);
		T* ptr = (T*)data;
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
	template <typename T>
	inline mat4<T> quat_to_mat4(quat<T> q)
	{
		mat4<T> mat;

		T twoX = static_cast<T>(2) * q.x;
		T twoY = static_cast<T>(2) * q.y;
		T twoZ = static_cast<T>(2) * q.z;

		T twoXX = twoX * q.x;
		T twoXY = twoX * q.y;
		T twoXZ = twoX * q.z;
		T twoXW = twoX * q.w;

		T twoYY = twoY * q.y;
		T twoYZ = twoY * q.z;
		T twoYW = twoY * q.w;

		T twoZZ = twoZ * q.z;
		T twoZW = twoZ * q.w;

		vec4f v0{ static_cast<T>(1.0) - (twoYY + twoZZ), twoXY - twoZW, twoXZ + twoYW, 0.0f };
		vec4f v1{ twoXY + twoZW, static_cast<T>(1.0) - (twoXX + twoZZ), twoYZ - twoXW, 0.0f };
		vec4f v2{ twoXZ - twoYW, twoYZ + twoXW, static_cast<T>(1.0) - (twoXX + twoYY), 0.0f };
		vec4f v3{ 0.0f, 0.0f, 0.0f, 1.0f };

		mat(v0, 0);
		mat(v1, 1);
		mat(v2, 2);
		mat(v3, 3);
		return mat;
	}

	template <typename T>
	inline T length_quat(quat<T>& q)
	{
		return std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
	}

	template <typename T>
	inline quat<T> normalise_quat(quat<T>& q)
	{
		quat<T> result;
		T length = length_quat(q);

		result.x = q.x / length;
		result.y = q.y / length;
		result.z = q.z / length;
		result.w = q.w / length;
		return result;
	}

	template <typename T>
	inline quat<T> linear_mix_quat(quat<T>& q1, quat<T>& q2, float u)
	{
		quat<T> result;
		result.x = q1.x * (T(1) - u) + q2.x * u;
		result.y = q1.y * (T(1) - u) + q2.y * u;
		result.z = q1.z * (T(1) - u) + q2.z * u;
		result.w = q1.w * (T(1) - u) + q2.w * u;
		return result;
	}

	template <typename T>
	inline quat<T> cubic_mix_quat(quat<T>& q1, quat<T>& q2, quat<T>& q3, quat<T>& q4, float u)
	{
		quat<T> retVec;
		T a0, a1, a2, a3, u_sqr;
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