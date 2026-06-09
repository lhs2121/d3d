#pragma once
#include "d3d11.h"
#include <commonlib/Interface.h>

struct SimpleVertex
{
	float4 POSITION;
	float2 TEXCOORD;
};

struct SpriteData
{
	float2 ratio = { 1.0f, 1.0f };
	float2 offset = { 0.0f, 0.0f };
};
