#include "pch.h"
#include "Math.h"
#include <math.h>

namespace math
{

void Vec4::operator*=(const Mat4& _other)
{
	Vec4 result;
	result.x = (x * _other.matrix[0][0]) + (y * _other.matrix[1][0]) + (z * _other.matrix[2][0]) + (w * _other.matrix[3][0]);
	result.y = (x * _other.matrix[0][1]) + (y * _other.matrix[1][1]) + (z * _other.matrix[2][1]) + (w * _other.matrix[3][1]);
	result.z = (x * _other.matrix[0][2]) + (y * _other.matrix[1][2]) + (z * _other.matrix[2][2]) + (w * _other.matrix[3][2]);
	result.w = (x * _other.matrix[0][3]) + (y * _other.matrix[1][3]) + (z * _other.matrix[2][3]) + (w * _other.matrix[3][3]);

	*this = result;
}

Vec4 Vec4::operator*(const Mat4& _other)
{
	Vec4 result;
	result.x = (x * _other.matrix[0][0]) + (y * _other.matrix[1][0]) + (z * _other.matrix[2][0]) + (w * _other.matrix[3][0]);
	result.y = (x * _other.matrix[0][1]) + (y * _other.matrix[1][1]) + (z * _other.matrix[2][1]) + (w * _other.matrix[3][1]);
	result.z = (x * _other.matrix[0][2]) + (y * _other.matrix[1][2]) + (z * _other.matrix[2][2]) + (w * _other.matrix[3][2]);
	result.w = (x * _other.matrix[0][3]) + (y * _other.matrix[1][3]) + (z * _other.matrix[2][3]) + (w * _other.matrix[3][3]);

	return result;
}

Vec4 Vec4::normalize(Vec4& _other)
{
	Vec4 result;
	result = _other;
	float length = static_cast<float>(sqrt(_other.x * _other.x + _other.y * _other.y + _other.z * _other.z));

	result.x /= length;
	result.y /= length;
	result.z /= length;
	return result;
}

void Vec4::normalize()
{
	float length = static_cast<float>(sqrt(x * x + y * y + z * z));
	x /= length;
	y /= length;
	z /= length;
}

float Vec4::distance(Vec4& __other) const
{
	Vec4 dis = *this - __other;

	return (float)sqrt((dis.x * dis.x) + (dis.y * dis.y));
}

void Vec4::rotate(float __radianian)
{
	Mat4 rz;
	rz.RotationZ(__radianian);

	*this *= rz;
}

Vec4 Vec4::resolution(float _force, float __radianian)
{
	return { _force * cosf(__radianian),_force * sinf(__radianian) };
}

float Vec4::dot(Vec4& Left, Vec4& Right)
{
	return Left.x * Right.x + Left.y * Right.y + Left.z * Right.z;
}

Vec4 Vec4::cross(Vec4& Left, Vec4& Right)
{
	float x = Left.y * Right.z - Left.z * Right.y;
	float y = Left.z * Right.x - Left.x * Right.z;
	float z = Left.x * Right.y - Left.y * Right.x;

	return { x,y,z,1 };
}

Mat4::Mat4(const Mat4& _other)
{
	for (int i = 0; i < 16; i++)
	{
		matrix1D[i] = _other.matrix1D[i];
	}
}

Mat4 Mat4::operator*(const Mat4& _other)
{
	Mat4 Result;
	Result.Zero();

	for (size_t r = 0; r < 4; r++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			for (size_t i = 0; i < 4; i++)
			{
				Result.matrix[r][j] += matrix[r][i] * _other.matrix[i][j];
			}
		}
	}

	return Result;
}

void Mat4::operator*=(const Mat4& _other)
{
	for (size_t r = 0; r < 4; r++)
	{
		for (size_t j = 0; j < 4; j++)
		{
			for (size_t i = 0; i < 4; i++)
			{
				matrix[r][j] += matrix[r][i] * _other.matrix[i][j];
			}
		}
	}
}

void Mat4::Identity()
{
	Zero();
	matrix[0][0] = 1.0f;
	matrix[1][1] = 1.0f;
	matrix[2][2] = 1.0f;
	matrix[3][3] = 1.0f;
}

void Mat4::Zero()
{
	for (int i = 0; i < 16; i++)
	{
		matrix1D[i] = 0.0f;
	}
}

void Mat4::TransPose()
{
	for (int x = 0; x < 4; x++)
	{
		for (int y = x + 1; y < 4; y++)
		{
			float temp = matrix[x][y];
			matrix[x][y] = matrix[y][x];
			matrix[y][x] = temp;
		}
	}
}


void Mat4::Position(const Vec4& _other)
{
	Identity();
	matrix[3][0] = _other.x;
	matrix[3][1] = _other.y;
	matrix[3][2] = _other.z;
}


void Mat4::Scale(const Vec4& _other)
{
	Identity();
	matrix[0][0] = _other.x;
	matrix[1][1] = _other.y;
	matrix[2][2] = _other.z;
}

void Mat4::Rotation(const Vec4& _degreeree)
{
	Identity();

	float _radianianX = _degreeree.x * Deg2Rad;
	float _radianianY = _degreeree.y * Deg2Rad;
	float _radianianZ = _degreeree.z * Deg2Rad;

	Mat4 ZRot;
	ZRot.RotationZ(_radianianZ);
	Mat4 YRot;
	YRot.RotationY(_radianianY);
	Mat4 XRot;
	XRot.RotationX(_radianianX);

	*this = ZRot * XRot * YRot;
}

void Mat4::RotationX(const float _radian)
{
	matrix[1][1] = cosf(_radian);
	matrix[1][2] = -sinf(_radian);
	matrix[2][1] = sinf(_radian);
	matrix[2][2] = cosf(_radian);
}

void Mat4::RotationY(const float _radian)
{
	matrix[0][0] = cosf(_radian);
	matrix[0][2] = -sinf(_radian);
	matrix[2][0] = sinf(_radian);
	matrix[2][2] = cosf(_radian);
}

void Mat4::RotationZ(const float _radian)
{
	matrix[0][0] = cosf(_radian);
	matrix[0][1] = sinf(_radian);
	matrix[1][0] = -sinf(_radian);
	matrix[1][1] = cosf(_radian);
}

void Mat4::View(Vec4& EyePos, Vec4& EyeDir, Vec4& EyeUp)
{
	Identity();

	EyeDir.normalize();
	EyeUp.normalize();

	Vec4 EyeRight = Vec4::cross(EyeUp, EyeDir);

	matrix[0][0] = EyeRight.x;
	matrix[0][1] = EyeRight.y;
	matrix[0][2] = EyeRight.z;

	matrix[1][0] = EyeUp.x;
	matrix[1][1] = EyeUp.y;
	matrix[1][2] = EyeUp.z;

	matrix[2][0] = EyeDir.x;
	matrix[2][1] = EyeDir.y;
	matrix[2][2] = EyeDir.z;

	TransPose();

	Vec4 Pos = -EyePos;
	matrix[3][0] = Vec4::dot(EyeRight, Pos);
	matrix[3][1] = Vec4::dot(EyeUp, Pos);
	matrix[3][2] = Vec4::dot(EyeDir, Pos);
}
void Mat4::Perspective(float FovYDegree, float Width, float Height, float Near, float Far)
{
	Identity();

	float FovY_radian = FovYDegree * Deg2Rad;
	float d = 1 / tanf(FovY_radian / 2);
	float InverseAspect = Height / Width;

	matrix[0][0] = d * InverseAspect;
	matrix[1][1] = d;
	matrix[2][2] = Far / (Far - Near);
	matrix[3][2] = (Far * Near) / (Near - Far);
	matrix[2][3] = 1;
	matrix[3][3] = 0;
}
void Mat4::Orthographic(float Width, float Height, float Near, float Far)
{
	Identity();

	matrix[0][0] = 2 / Width;
	matrix[1][1] = 2 / Height;
	matrix[2][2] = 1 / (Far - Near);
	matrix[3][2] = Near / (Near - Far);
}

int digits(int _num)
{
	int result = 0;

	while (_num >= 1)
	{
		_num /= 10;
		result++;
	}
	return result;
}

float clamp(float _num, float _max, float _min)
{
	if (_num >= _max)
	{
		return _max;
	}
	if (_num <= _min)
	{
		return _min;
	}
	return _num;
}

}

