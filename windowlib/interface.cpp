#include "pch.h"
#include "Interface.h"
#include "WindowObject.h"

void CreateWindowObject(IWindowObject** ppWindowObject)
{
    *ppWindowObject = new CWindowObject;
}

void DeleteWindowObject(IWindowObject* pWindowObject)
{
    CWindowObject* pCast = (CWindowObject*)pWindowObject;
    delete pCast;
}
