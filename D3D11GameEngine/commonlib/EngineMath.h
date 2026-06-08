#pragma once
#include "Interface.h"

constexpr float PI = 3.141592653f;
constexpr float Deg2Rad = 3.141592653f / 180.0f;

struct COMMONLIB_API math
{
	static int digits(int _num);
	static float clamp(float _num, float _max, float _min);
};
struct float2
{
	float2(float _x = 0, float _y = 0) : x(_x), y(_y) {}

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

	float2 operator+(const float2& _other) const
	{
		return { x + _other.x ,y + _other.y };
	}

	float2 operator+(const float2&& _other) const
	{
		return { x + _other.x ,y + _other.y };
	}

	float2 operator-(const float2& _other) const
	{
		return { x - _other.x ,y - _other.y };
	}

	float2 operator-(const float2&& _other) const
	{
		return { x - _other.x ,y - _other.y };
	}

	float2 operator-() const
	{
		return { -x,-y };
	}

	float2 operator*(const float2& _other) const
	{
		return { x * _other.x ,y * _other.y };
	}

	float2 operator*(const float2&& _other) const
	{
		return { x * _other.x ,y * _other.y };
	}

	float2 operator*(const float _other) const
	{
		return { x * _other,y * _other };
	}

	float2 operator/(const float2& _other) const
	{
		return { x / _other.x ,y / _other.y };
	}

	float2 operator/(const float2&& _other) const
	{
		return { x / _other.x ,y / _other.y };
	}

	float2 operator/(float _other) const
	{
		return { x / _other ,y / _other };
	}

	void operator+=(const float2& _other)
	{
		x += _other.x;
		y += _other.y;
	}

	void operator+=(const float2&& _other)
	{
		x += _other.x;
		y += _other.y;
	}

	void operator-=(const float2& _other)
	{
		x -= _other.x;
		y -= _other.y;
	}

	void operator-=(const float2&& _other)
	{
		x -= _other.x;
		y -= _other.y;
	}

	void operator*=(const float2& _other)
	{
		x *= _other.x;
		y *= _other.y;
	}

	void operator*=(const float2&& _other)
	{
		x *= _other.x;
		y *= _other.y;
	}

	void operator*=(const float _other)
	{
		x *= _other;
		y *= _other;
	}

	void operator/=(const float2& _other)
	{
		x /= _other.x;
		y /= _other.y;
	}

	void operator/=(const float2&& _other)
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

struct float3
{
	float3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}

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

	float3 operator+(const float3& _other) const
	{
		return { x + _other.x ,y + _other.y ,z + _other.z };
	}

	float3 operator+(const float3&& _other) const
	{
		return { x + _other.x ,y + _other.y ,z + _other.z };
	}

	float3 operator-(const float3& _other) const
	{
		return { x - _other.x ,y - _other.y ,z - _other.z };
	}

	float3 operator-(const float3&& _other) const
	{
		return { x - _other.x ,y - _other.y ,z - _other.z };
	}

	float3 operator-() const
	{
		return { -x,-y,-z };
	}

	float3 operator*(const float3& _other) const
	{
		return { x * _other.x ,y * _other.y ,z * _other.z };
	}

	float3 operator*(const float3&& _other) const
	{
		return { x * _other.x ,y * _other.y ,z * _other.z };
	}

	float3 operator*(const float _other) const
	{
		return { x * _other,y * _other,z * _other };
	}

	float3 operator/(const float3& _other) const
	{
		return { x / _other.x ,y / _other.y ,z / _other.z };
	}

	float3 operator/(const float3&& _other) const
	{
		return { x / _other.x ,y / _other.y ,z / _other.z };
	}

	float3 operator/(float _other) const
	{
		return { x / _other ,y / _other ,z / _other };
	}

	void operator+=(const float3& _other)
	{
		x += _other.x;
		y += _other.y;
		z += _other.z;
	}

	void operator+=(const float3&& _other)
	{
		x += _other.x;
		y += _other.y;
		z += _other.z;
	}

	void operator-=(const float3& _other)
	{
		x -= _other.x;
		y -= _other.y;
		z -= _other.z;
	}

	void operator-=(const float3&& _other)
	{
		x -= _other.x;
		y -= _other.y;
		z -= _other.z;
	}

	void operator*=(const float3& _other)
	{
		x *= _other.x;
		y *= _other.y;
		z *= _other.z;
	}

	void operator*=(const float3&& _other)
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

	void operator/=(const float3& _other)
	{
		x /= _other.x;
		y /= _other.y;
		z /= _other.z;
	}

	void operator/=(const float3&& _other)
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
struct float4x4;
struct COMMONLIB_API float4
{
	float4(float _x = 0, float _y = 0, float _z = 0, float _w = 1) : x(_x), y(_y), z(_z), w(_w) {}

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

	static float4 resolution(float _force, float __radianian);

	static float4 normalize(float4& __other);

	static float dot(float4& _left, float4& _right);

	static float4 cross(float4& _left, float4& _right);

	void normalize();

	float distance(float4& __other) const;

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

	float4 operator+(const float4& _other) const
	{
		return { x + _other.x ,y + _other.y ,z + _other.z ,w };
	}

	float4 operator+(const float4&& _other) const
	{
		return { x + _other.x ,y + _other.y ,z + _other.z ,w };
	}

	float4 operator-(const float4& _other) const
	{
		return { x - _other.x ,y - _other.y ,z - _other.z ,w };
	}

	float4 operator-(const float4&& _other) const
	{
		return { x - _other.x ,y - _other.y ,z - _other.z ,w };
	}

	float4 operator-() const
	{
		return { -x,-y,-z ,w };
	}

	float4 operator*(const float4& _other) const
	{
		return { x * _other.x ,y * _other.y ,z * _other.z ,w };
	}

	float4 operator*(const float4&& _other) const
	{
		return { x * _other.x ,y * _other.y ,z * _other.z ,w };
	}

	float4 operator*(const float _other) const
	{
		return { x * _other,y * _other,z * _other,w };
	}

	float4 operator/(const float4& _other) const
	{
		return { x / _other.x ,y / _other.y ,z / _other.z ,w };
	}

	float4 operator/(const float4&& _other) const
	{
		return { x / _other.x ,y / _other.y ,z / _other.z ,w };
	}

	float4 operator/(float _other) const
	{
		return { x / _other ,y / _other ,z / _other ,w };
	}

	void operator+=(const float4& _other)
	{
		x += _other.x;
		y += _other.y;
		z += _other.z;
	}

	void operator+=(const float4&& _other)
	{
		x += _other.x;
		y += _other.y;
		z += _other.z;
	}

	void operator-=(const float4& _other)
	{
		x -= _other.x;
		y -= _other.y;
		z -= _other.z;
	}

	void operator-=(const float4&& _other)
	{
		x -= _other.x;
		y -= _other.y;
		z -= _other.z;
	}

	void operator*=(const float4& _other)
	{
		x *= _other.x;
		y *= _other.y;
		z *= _other.z;
	}

	void operator*=(const float4&& _other)
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

	void operator/=(const float4& _other)
	{
		x /= _other.x;
		y /= _other.y;
		z /= _other.z;
	}

	void operator/=(const float4&& _other)
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

	void operator*=(const float4x4& _other);

	float4 operator*(const float4x4& _other);

};

struct COMMONLIB_API float4x4
{
	float4x4() {}
	float4x4(const float4x4& _other);
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

	float4x4 operator*(const float4x4& _other);
	void operator*=(const float4x4& _other);

	void Identity();
	void Zero();
	void TransPose();

	void Position(const float4& _other);
	void Scale(const float4& _other);
	void Rotation(const float4& _degree);

	void RotationX(const float _radian);
	void RotationY(const float _radian);
	void RotationZ(const float _radian);

	void View(float4& _eyePos, float4& _eyeDir, float4& _eyeUp);
	void Perspective(float _fovYDegree, float width, float height, float _near, float _far);
	void Orthographic(float _width, float _height, float _near, float _far);
};