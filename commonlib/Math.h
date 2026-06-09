#pragma once
#include "Interface.h"

constexpr float PI = 3.141592653f;
constexpr float Deg2Rad = 3.141592653f / 180.0f;

namespace math
{
	COMMONLIB_API int digits(int _num);
	COMMONLIB_API float clamp(float _num, float _max, float _min);

struct Vec2
{
	Vec2(float _x = 0, float _y = 0) : x(_x), y(_y) {}

	float x;
	float y;

	float hx() const
	{
		return x / 2;
	}

	float hy() const
	{
		return y / 2;
	}

	int ix() const
	{
		return static_cast<int>(x);
	}

	int iy() const
	{
		return static_cast<int>(y);
	}

	int ihx() const
	{
		return static_cast<int>(x / 2);
	}

	int ihy() const
	{
		return static_cast<int>(y / 2);
	}

	Vec2 operator+(const Vec2& _other) const
	{
		return { x + _other.x ,y + _other.y };
	}

	Vec2 operator+(const Vec2&& _other) const
	{
		return { x + _other.x ,y + _other.y };
	}

	Vec2 operator-(const Vec2& _other) const
	{
		return { x - _other.x ,y - _other.y };
	}

	Vec2 operator-(const Vec2&& _other) const
	{
		return { x - _other.x ,y - _other.y };
	}

	Vec2 operator-() const
	{
		return { -x,-y };
	}

	Vec2 operator*(const Vec2& _other) const
	{
		return { x * _other.x ,y * _other.y };
	}

	Vec2 operator*(const Vec2&& _other) const
	{
		return { x * _other.x ,y * _other.y };
	}

	Vec2 operator*(const float _other) const
	{
		return { x * _other,y * _other };
	}

	Vec2 operator/(const Vec2& _other) const
	{
		return { x / _other.x ,y / _other.y };
	}

	Vec2 operator/(const Vec2&& _other) const
	{
		return { x / _other.x ,y / _other.y };
	}

	Vec2 operator/(float _other) const
	{
		return { x / _other ,y / _other };
	}

	void operator+=(const Vec2& _other)
	{
		x += _other.x;
		y += _other.y;
	}

	void operator+=(const Vec2&& _other)
	{
		x += _other.x;
		y += _other.y;
	}

	void operator-=(const Vec2& _other)
	{
		x -= _other.x;
		y -= _other.y;
	}

	void operator-=(const Vec2&& _other)
	{
		x -= _other.x;
		y -= _other.y;
	}

	void operator*=(const Vec2& _other)
	{
		x *= _other.x;
		y *= _other.y;
	}

	void operator*=(const Vec2&& _other)
	{
		x *= _other.x;
		y *= _other.y;
	}

	void operator*=(const float _other)
	{
		x *= _other;
		y *= _other;
	}

	void operator/=(const Vec2& _other)
	{
		x /= _other.x;
		y /= _other.y;
	}

	void operator/=(const Vec2&& _other)
	{
		x /= _other.x;
		y /= _other.y;
	}

	void operator/=(const float _other)
	{
		x /= _other;
		y /= _other;
	}

};

struct Vec3
{
	Vec3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}

	float x;
	float y;
	float z;

	float hx() const
	{
		return x / 2;
	}

	float hy() const
	{
		return y / 2;
	}

	int ix() const
	{
		return static_cast<int>(x);
	}

	int iy() const
	{
		return static_cast<int>(y);
	}

	int ihx() const
	{
		return static_cast<int>(x / 2);
	}

	int ihy() const
	{
		return static_cast<int>(y / 2);
	}

	Vec3 operator+(const Vec3& _other) const
	{
		return { x + _other.x ,y + _other.y ,z + _other.z };
	}

	Vec3 operator+(const Vec3&& _other) const
	{
		return { x + _other.x ,y + _other.y ,z + _other.z };
	}

	Vec3 operator-(const Vec3& _other) const
	{
		return { x - _other.x ,y - _other.y ,z - _other.z };
	}

	Vec3 operator-(const Vec3&& _other) const
	{
		return { x - _other.x ,y - _other.y ,z - _other.z };
	}

	Vec3 operator-() const
	{
		return { -x,-y,-z };
	}

	Vec3 operator*(const Vec3& _other) const
	{
		return { x * _other.x ,y * _other.y ,z * _other.z };
	}

	Vec3 operator*(const Vec3&& _other) const
	{
		return { x * _other.x ,y * _other.y ,z * _other.z };
	}

	Vec3 operator*(const float _other) const
	{
		return { x * _other,y * _other,z * _other };
	}

	Vec3 operator/(const Vec3& _other) const
	{
		return { x / _other.x ,y / _other.y ,z / _other.z };
	}

	Vec3 operator/(const Vec3&& _other) const
	{
		return { x / _other.x ,y / _other.y ,z / _other.z };
	}

	Vec3 operator/(float _other) const
	{
		return { x / _other ,y / _other ,z / _other };
	}

	void operator+=(const Vec3& _other)
	{
		x += _other.x;
		y += _other.y;
		z += _other.z;
	}

	void operator+=(const Vec3&& _other)
	{
		x += _other.x;
		y += _other.y;
		z += _other.z;
	}

	void operator-=(const Vec3& _other)
	{
		x -= _other.x;
		y -= _other.y;
		z -= _other.z;
	}

	void operator-=(const Vec3&& _other)
	{
		x -= _other.x;
		y -= _other.y;
		z -= _other.z;
	}

	void operator*=(const Vec3& _other)
	{
		x *= _other.x;
		y *= _other.y;
		z *= _other.z;
	}

	void operator*=(const Vec3&& _other)
	{
		x *= _other.x;
		y *= _other.y;
		z *= _other.z;
	}

	void operator*=(const float _other)
	{
		x *= _other;
		y *= _other;
		z *= _other;
	}

	void operator/=(const Vec3& _other)
	{
		x /= _other.x;
		y /= _other.y;
		z /= _other.z;
	}

	void operator/=(const Vec3&& _other)
	{
		x /= _other.x;
		y /= _other.y;
		z /= _other.z;
	}

	void operator/=(const float _other)
	{
		x /= _other;
		y /= _other;
		z /= _other;
	}
};
struct Mat4;
struct COMMONLIB_API Vec4
{
	Vec4(float _x = 0, float _y = 0, float _z = 0, float _w = 1) : x(_x), y(_y), z(_z), w(_w) {}

	union
	{
		struct
		{
			float x;
			float y;
			float z;
			float w;
		};

		struct
		{
			float r;
			float g;
			float b;
			float a;
		};
	};

	static Vec4 resolution(float _force, float __radianian);

	static Vec4 normalize(Vec4& __other);

	static float dot(Vec4& _left, Vec4& _right);

	static Vec4 cross(Vec4& _left, Vec4& _right);

	void normalize();

	float distance(Vec4& __other) const;

	void rotate(float __radianian);

	float hx() const
	{
		return x / 2;
	}

	float hy() const
	{
		return y / 2;
	}

	int ix() const
	{
		return static_cast<int>(x);
	}

	int iy() const
	{
		return static_cast<int>(y);
	}

	int ihx() const
	{
		return static_cast<int>(x / 2);
	}

	int ihy() const
	{
		return static_cast<int>(y / 2);
	}

	Vec4 operator+(const Vec4& _other) const
	{
		return { x + _other.x ,y + _other.y ,z + _other.z ,w };
	}

	Vec4 operator+(const Vec4&& _other) const
	{
		return { x + _other.x ,y + _other.y ,z + _other.z ,w };
	}

	Vec4 operator-(const Vec4& _other) const
	{
		return { x - _other.x ,y - _other.y ,z - _other.z ,w };
	}

	Vec4 operator-(const Vec4&& _other) const
	{
		return { x - _other.x ,y - _other.y ,z - _other.z ,w };
	}

	Vec4 operator-() const
	{
		return { -x,-y,-z ,w };
	}

	Vec4 operator*(const Vec4& _other) const
	{
		return { x * _other.x ,y * _other.y ,z * _other.z ,w };
	}

	Vec4 operator*(const Vec4&& _other) const
	{
		return { x * _other.x ,y * _other.y ,z * _other.z ,w };
	}

	Vec4 operator*(const float _other) const
	{
		return { x * _other,y * _other,z * _other,w };
	}

	Vec4 operator/(const Vec4& _other) const
	{
		return { x / _other.x ,y / _other.y ,z / _other.z ,w };
	}

	Vec4 operator/(const Vec4&& _other) const
	{
		return { x / _other.x ,y / _other.y ,z / _other.z ,w };
	}

	Vec4 operator/(float _other) const
	{
		return { x / _other ,y / _other ,z / _other ,w };
	}

	void operator+=(const Vec4& _other)
	{
		x += _other.x;
		y += _other.y;
		z += _other.z;
	}

	void operator+=(const Vec4&& _other)
	{
		x += _other.x;
		y += _other.y;
		z += _other.z;
	}

	void operator-=(const Vec4& _other)
	{
		x -= _other.x;
		y -= _other.y;
		z -= _other.z;
	}

	void operator-=(const Vec4&& _other)
	{
		x -= _other.x;
		y -= _other.y;
		z -= _other.z;
	}

	void operator*=(const Vec4& _other)
	{
		x *= _other.x;
		y *= _other.y;
		z *= _other.z;
	}

	void operator*=(const Vec4&& _other)
	{
		x *= _other.x;
		y *= _other.y;
		z *= _other.z;
	}

	void operator*=(const float _other)
	{
		x *= _other;
		y *= _other;
		z *= _other;
	}

	void operator/=(const Vec4& _other)
	{
		x /= _other.x;
		y /= _other.y;
		z /= _other.z;
	}

	void operator/=(const Vec4&& _other)
	{
		x /= _other.x;
		y /= _other.y;
		z /= _other.z;
	}

	void operator/=(const float _other)
	{
		x /= _other;
		y /= _other;
		z /= _other;
	}

	void operator*=(const Mat4& _other);

	Vec4 operator*(const Mat4& _other);

};

struct COMMONLIB_API Mat4
{
	Mat4() {}
	Mat4(const Mat4& _other);
	union
	{
		float matrix[4][4] =
		{
			{ 1.0f, 0.0f, 0.0f, 0.0f},
			{ 0.0f, 1.0f, 0.0f, 0.0f},
			{ 0.0f, 0.0f, 1.0f, 0.0f},
			{ 0.0f, 0.0f, 0.0f, 1.0f}
		};

		float matrix1D[16];
	};

	Mat4 operator*(const Mat4& _other);
	void operator*=(const Mat4& _other);

	void Identity();
	void Zero();
	void TransPose();

	void Position(const Vec4& _other);
	void Scale(const Vec4& _other);
	void Rotation(const Vec4& _degree);

	void RotationX(const float _radian);
	void RotationY(const float _radian);
	void RotationZ(const float _radian);

	void View(Vec4& _eyePos, Vec4& _eyeDir, Vec4& _eyeUp);
	void Perspective(float _fovYDegree, float width, float height, float _near, float _far);
	void Orthographic(float _width, float _height, float _near, float _far);
};
}
