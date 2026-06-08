#include "pch.h"
#include "Interface.h"
#include "Renderer.h"

void CreateRenderer(IRenderer** ppRenderer)
{
    *ppRenderer = new Renderer();
}

void DeleteRenderer(IRenderer* pRenderer)
{
    Renderer* pCast = (Renderer*)pRenderer;
    delete pCast;
}
