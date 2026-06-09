#include "pch.h"
#include "Interface.h"
#include "Renderer.h"

void CreateRenderer(IRenderer** ppRenderer)
{
    *ppRenderer = new Renderer();
}

void DeleteRenderer(IRenderer* renderer)
{
	delete static_cast<Renderer*>(renderer);
}
