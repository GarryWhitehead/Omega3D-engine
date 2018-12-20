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

	private:

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
}