#pragma once
#pragma warning(disable:4099)

#ifdef COMMONLIB_EXPORTS 
#define COMMONLIB_API __declspec(dllexport)
#else
#define COMMONLIB_API __declspec(dllimport)
#endif

#include <Windows.h>
#include "Math.h"

namespace Debug
{
	extern "C" COMMONLIB_API void CrtSetBreakAlloc(UINT num);
	extern "C" COMMONLIB_API void CrtSetDbgFlag();
	extern "C" COMMONLIB_API void MsgBoxAssert(const char* errorMsg);
}

struct ITimer
{
	virtual void  Initialize() = 0;
	virtual void  Reset() = 0;
	virtual float GetElapsedSeconds() = 0;
};

extern "C" COMMONLIB_API void CreateTimer(ITimer** ppTimer);
extern "C" COMMONLIB_API void DeleteTimer(ITimer* timer);
