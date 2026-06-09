#pragma once

#ifdef COLLIB_EXPORTS
#define COLLIB_API __declspec(dllexport)
#else
#define COLLIB_API __declspec(dllimport)
#endif

namespace collib
{
	struct AABB
	{
		float centerX = 0.0f;
		float centerY = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
	};

	COLLIB_API AABB MakeAABB(float centerX, float centerY, float width, float height);
	COLLIB_API AABB Inset(const AABB& box, float insetX, float insetY);
	COLLIB_API float Left(const AABB& box);
	COLLIB_API float Right(const AABB& box);
	COLLIB_API float Top(const AABB& box);
	COLLIB_API float Bottom(const AABB& box);
	COLLIB_API bool Intersects(const AABB& a, const AABB& b);
	COLLIB_API bool ContainsPoint(const AABB& box, float pointX, float pointY);
}
