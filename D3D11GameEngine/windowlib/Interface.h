#pragma once
#include "Windows.h"

#ifdef WINDOWLIB_EXPORTS
#define WINDOWLIB_API __declspec(dllexport)
#else
#define WINDOWLIB_API __declspec(dllimport)
#endif 

struct IEngine
{
	virtual void EngineUpdate() = 0;
	virtual void EngineRelease() = 0;
};

struct IWindowObject
{
	virtual void Initialize(const char* szTitle, float posX, float posY, float width, float height, const HINSTANCE hInstance, IEngine* pEngine) = 0;

    virtual void MessageLoop() = 0;

	virtual const char* GetTitle() = 0;

	virtual UINT GetDpi() = 0;

	virtual float GetWidth() = 0;

	virtual float GetHeight() = 0;

	virtual float GetPosX() = 0;

	virtual float GetPosY() = 0;

	virtual HINSTANCE GetHINSTANCE() = 0;

	virtual HWND* GetHWND() = 0;

};

extern "C" WINDOWLIB_API void CreateWindowObject(IWindowObject** ppWindowObject);

extern "C" WINDOWLIB_API void DeleteWindowObject(IWindowObject* pWindowObject);
