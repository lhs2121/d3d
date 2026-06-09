#pragma once
#include "d3d11.h"
#include <commonlib/Interface.h>

struct SimpleVertex
{
	math::Vec4 POSITION;
	math::Vec2 TEXCOORD;
};

struct SpriteData
{
	math::Vec2 ratio = { 1.0f, 1.0f };
	math::Vec2 offset = { 0.0f, 0.0f };
	math::Vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
};
