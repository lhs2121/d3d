#include "pch.h"
#include "EngineMath.h"
#include <math.h>

void float4::operator*=(const float4x4& _other)
{
	float4 result;
	result.x = (x * _other.matrix[0][0]) + (y * _other.matrix[1][0]) + (z * _other.matrix[2][0]) + (w * _other.matrix[3][0]);
	result.y = (x * _other.matrix[0][1]) + (y * _other.matrix[1][1]) + (z * _other.matrix[2][1]) + (w * _other.matrix[3][1]);
	result.z = (x * _other.matrix[0][2]) + (y * _other.matrix[1][2]) + (z * _other.matrix[2][2]) + (w * _other.matrix[3][2]);
	result.w = (x * _other.matrix[0][3]) + (y * _other.matrix[1][3]) + (z * _other.matrix[2][3]) + (w * _other.matrix[3][3]);

	*this = result;
}

float4 float4::operator*(const float4x4& _other)
{
	float4 result;
	result.x = (x * _other.matrix[0][0]) + (y * _other.matrix[1][0]) + (z * _other.matrix[2][0]) + (w * _other.matrix[3][0]);
	result.y = (x * _other.matrix[0][1]) + (y * _other.matrix[1][1]) + (z * _other.matrix[2][1]) + (w * _other.matrix[3][1]);
	result.z = (x * _other.matrix[0][2]) + (y * _other.matrix[1][2]) + (z * _other.matrix[2][2]) + (w * _other.matrix[3][2]);
	result.w = (x * _other.matrix[0][3]) + (y * _other.matrix[1][3]) + (z * _other.matrix[2][3]) + (w * _other.matrix[3][3]);

	return result;
}

float4 float4::normalize(float4& _other)
{
	float4 result;
	result = _other;
	float length = static_cast<float>(sqrt(_other.x * _other.x + _other.y * _other.y + _other.z * _other.z));

	result.x /= length;
	result.y /= length;
	result.z /= length;
	return result;
}

void float4::normalize()
{
	float length = static_cast<float>(sqrt(x * x + y * y + z * z));
	x /= length;
	y /= length;
	z /= length;
}

float float4::distance(float4& __other) const
{
	float4 dis = *this - __other;

	return (float)sqrt((dis.x * dis.x) + (dis.y * dis.y));
}

void float4::rotate(float __radianian)
{
	float4x4 rz;
	rz.RotationZ(__radianian);

	*this *= rz;
}

float4 float4::resolution(float _force, float __radianian)
{
	return { _force * cosf(__radianian),_force * sinf(__radianian) };
}

float float4::dot(float4& Left, float4& Right)
{
	return Left.x * Right.x + Left.y * Right.y + Left.z * Right.z;
}

float4 float4::cross(float4& Left, float4& Right)
{
	float x = Left.y * Right.z - Left.z * Right.y;
	float y = Left.z * Right.x - Left.x * Right.z;
	float z = Left.x * Right.y - Left.y * Right.x;

	return { x,y,z,1 };
}

float4x4::float4x4(const float4x4& _other)
{
	for (int i = 0; i < 16; i++)
	{
		matrix1D[i] = _other.matrix1D[i];
	}
}

float4x4 float4x4::operator*(const float4x4& _other)
{
	float4x4 Result;
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

void float4x4::operator*=(const float4x4& _other)
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

void float4x4::Identity()
{
	Zero();
	matrix[0][0] = 1.0f;
	matrix[1][1] = 1.0f;
	matrix[2][2] = 1.0f;
	matrix[3][3] = 1.0f;
}

void float4x4::Zero()
{
	for (int i = 0; i < 16; i++)
	{
		matrix1D[i] = 0.0f;
	}
}

void float4x4::TransPose()
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


void float4x4::Position(const float4& _other)
{
	Identity();
	matrix[3][0] = _other.x;
	matrix[3][1] = _other.y;
	matrix[3][2] = _other.z;
}


void float4x4::Scale(const float4& _other)
{
	Identity();
	matrix[0][0] = _other.x;
	matrix[1][1] = _other.y;
	matrix[2][2] = _other.z;
}

void float4x4::Rotation(const float4& _degreeree)
{
	Identity();

	float _radianianX = _degreeree.x * Deg2Rad;
	float _radianianY = _degreeree.y * Deg2Rad;
	float _radianianZ = _degreeree.z * Deg2Rad;

	float4x4 ZRot;
	ZRot.RotationZ(_radianianZ);
	float4x4 YRot;
	YRot.RotationY(_radianianY);
	float4x4 XRot;
	XRot.RotationX(_radianianX);

	*this = ZRot * XRot * YRot;
}

void float4x4::RotationX(const float _radian)
{
	matrix[1][1] = cosf(_radian);
	matrix[1][2] = -sinf(_radian);
	matrix[2][1] = sinf(_radian);
	matrix[2][2] = cosf(_radian);
}

void float4x4::RotationY(const float _radian)
{
	matrix[0][0] = cosf(_radian);
	matrix[0][2] = -sinf(_radian);
	matrix[2][0] = sinf(_radian);
	matrix[2][2] = cosf(_radian);
}

void float4x4::RotationZ(const float _radian)
{
	matrix[0][0] = cosf(_radian);
	matrix[0][1] = sinf(_radian);
	matrix[1][0] = -sinf(_radian);
	matrix[1][1] = cosf(_radian);
}

void float4x4::View(float4& EyePos, float4& EyeDir, float4& EyeUp)
{
	Identity();

	EyeDir.normalize();
	EyeUp.normalize();

	float4 EyeRight = float4::cross(EyeUp, EyeDir);

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

	float4 Pos = -EyePos;
	matrix[3][0] = float4::dot(EyeRight, Pos);
	matrix[3][1] = float4::dot(EyeUp, Pos);
	matrix[3][2] = float4::dot(EyeDir, Pos);
}
void float4x4::Perspective(float FovYDegree, float Width, float Height, float Near, float Far)
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
void float4x4::Orthographic(float Width, float Height, float Near, float Far)
{
	Identity();

	matrix[0][0] = 2 / Width;
	matrix[1][1] = 2 / Height;
	matrix[2][2] = 1 / (Far - Near);
	matrix[3][2] = Near / (Near - Far);
}

int math::digits(int _num)
{
	int result = 0;

	while (_num >= 1)
	{
		_num /= 10;
		result++;
	}
	return result;
}

float math::clamp(float _num, float _max, float _min)
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

