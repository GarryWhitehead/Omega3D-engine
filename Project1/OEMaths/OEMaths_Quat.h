#pragma once

namespace OEMaths
{

	template <typename Typename>
	class quat
	{
	public:

		quat() :
			x(static_cast<Typename>(0)),
			y(static_cast<Typename>(0)),
			z(static_cast<Typename>(0)),
			w(static_cast<Typename>(0))
		{}

		quat(Typename n) :
			x(n),
			y(n),
			z(n),
			w(n)
		{}

		quat(Typename in_x, Typename in_y, Typename in_z, Typename in_w) :
			x(in_x),
			y(in_y),
			z(in_z),
			w(in_w)
		{}

		Typename x;
		Typename y;
		Typename z;
		Typename w;
	};

	using quatf = quat<float>;
	using quatd = quat<double>;


	// conversion functions
	template <typename Typename>
	quat<Typename> convert_quat(Typename* data)
	{
		assert(data != nullptr);

		vec4<Typename> qt;
		qt.x = *data;
		data += sizeof(Typename);
		qt.y = *data;
		data += sizeof(Typename);
		qt.z = *data;
		data += sizeof(Typename);
		qt.w = *data;
		return qt;
	}

	// conversion
	template <typename Typename>
	inline mat4<Typename> quat_to_mat4(quat<Typename> q)
	{
		mat4<Typename> mat;

		Typename twoX = static_cast<Typename>(2.0) * q.x;
		Typename twoY = static_cast<Typename>(2.0) * q.y;
		Typename twoZ = static_cast<Typename>(2.0) * q.z;

		Typename twoXX = twoX * q.x;
		Typename twoXY = twoX * q.y;
		Typename twoXZ = twoX * q.z;
		Typename twoXW = twoX * q.w;

		Typename twoYY = twoY * q.y;
		Typename twoYZ = twoY * q.z;
		Typename twoYW = twoY * q.w;

		Typename twoZZ = twoZ * q.z;
		Typename twoZW = twoZ * q.w;

		vec4f v0{ static_cast<Typename>(1.0) - (twoYY + twoZZ), twoXY - twoZW, twoXZ + twoYW, 0.0f };
		vec4f v1{ twoXY + twoZW, static_cast<Typename>(1.0) - (twoXX + twoZZ), twoYZ - twoXW, 0.0f };
		vec4f v2{ twoXZ - twoYW, twoYZ + twoXW, static_cast<Typename>(1.0) - (twoXX + twoYY), 0.0f };
		vec4f v3{ 0.0f, 0.0f, 0.0f, 1.0f };

		mat(v0, 0);
		mat(v1, 1);
		mat(v2, 2);
		mat(v3, 3);
		return mat;
	}

	template <typename Typename>
	inline quat<Typename> length_quat(quat<Typename>& q)
	{
		return std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
	}

	template <typename Typename>
	inline quat<Typename> normalise_quat(quat<Typename>& q)
	{
		vec4<Typename> retVec;
		Typename length = length_quat(q);
		Typename invLength = static_cast<Typename>(1) / length;

		retVec.x = q.x * invLength;
		retVec.y = q.y * invLength;
		retVec.z = q.z * invLength;
		retVec.w = q.w * invLength;
		return retVec;
	}

	template <typename Typename>
	inline quat<Typename> linear_mix_quat(quat<Typename>& q1, quat<Typename>& q2, float u)
	{
		quat<Typename> retVec;
		retVec.x = q1.x * (1 - u) + q2.x * u;
		retVec.y = q1.y * (1 - u) + q2.y * u;
		retVec.z = q1.z * (1 - u) + q2.z * u;
		retVec.w = q1.w * (1 - u) + q2.w * u;
		return retVec;
	}

	template <typename Typename>
	inline quat<Typename> cubic_mix_quat(quat<Typename>& q1, quat<Typename>& q2, quat<Typename>& q3, quat<Typename>& q4, float u)
	{
		quat<Typename> retVec;
		Typename a0, a1, a2, a3, u_sqr;
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