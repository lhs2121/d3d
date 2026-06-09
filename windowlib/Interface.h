#pragma once
#include "Windows.h"

#ifdef WINDOWLIB_EXPORTS
#define WINDOWLIB_API __declspec(dllexport)
#else
#define WINDOWLIB_API __declspec(dllimport)
#endif 

struct IGameLoop
{
	virtual void Update() = 0;
	virtual void Release() = 0;
};

struct IWindow
{
	virtual void Initialize(const char* szTitle, float posX, float posY, float width, float height, const HINSTANCE hInstance, IGameLoop* gameLoop) = 0;

	virtual void RunMessageLoop() = 0;

	virtual const char* GetTitle() = 0;

	virtual UINT GetDpi() = 0;

	virtual float GetWidth() = 0;

	virtual float GetHeight() = 0;

	virtual float GetPosX() = 0;

	virtual float GetPosY() = 0;

	virtual HINSTANCE GetHINSTANCE() = 0;

	virtual HWND* GetHWND() = 0;

};

extern "C" WINDOWLIB_API void CreateWindowInstance(IWindow** ppWindow);

extern "C" WINDOWLIB_API void DeleteWindowInstance(IWindow* window);
