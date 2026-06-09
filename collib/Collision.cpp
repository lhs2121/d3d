#include "pch.h"
#include "Interface.h"

namespace
{
	float ClampPositiveSize(float value)
	{
		return value > 0.0f ? value : 0.0f;
	}
}

collib::AABB collib::MakeAABB(float centerX, float centerY, float width, float height)
{
	AABB box;
	box.centerX = centerX;
	box.centerY = centerY;
	box.width = ClampPositiveSize(width);
	box.height = ClampPositiveSize(height);
	return box;
}

collib::AABB collib::Inset(const AABB& box, float insetX, float insetY)
{
	const float width = box.width - insetX * 2.0f;
	const float height = box.height - insetY * 2.0f;
	return MakeAABB(box.centerX, box.centerY, width, height);
}

float collib::Left(const AABB& box)
{
	return box.centerX - box.width * 0.5f;
}

float collib::Right(const AABB& box)
{
	return box.centerX + box.width * 0.5f;
}

float collib::Top(const AABB& box)
{
	return box.centerY + box.height * 0.5f;
}

float collib::Bottom(const AABB& box)
{
	return box.centerY - box.height * 0.5f;
}

bool collib::Intersects(const AABB& a, const AABB& b)
{
	return Left(a) < Right(b) &&
		Right(a) > Left(b) &&
		Bottom(a) < Top(b) &&
		Top(a) > Bottom(b);
}

bool collib::ContainsPoint(const AABB& box, float pointX, float pointY)
{
	return pointX >= Left(box) &&
		pointX <= Right(box) &&
		pointY >= Bottom(box) &&
		pointY <= Top(box);
}

