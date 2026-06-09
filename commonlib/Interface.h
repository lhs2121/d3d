#pragma once
#pragma warning(disable:4099)

#ifdef COMMONLIB_EXPORTS 
#define COMMONLIB_API __declspec(dllexport)
#else
#define COMMONLIB_API __declspec(dllimport)
#endif

#include <Windows.h>
#include "EngineMath.h"

namespace Debug
{
	extern "C" COMMONLIB_API void CrtSetBreakAlloc(UINT num);
	extern "C" COMMONLIB_API void CrtSetDbgFlag();
	extern "C" COMMONLIB_API void MsgBoxAssert(const char* errorMsg);
}

struct ITimeObject
{
	virtual void  Initialize() = 0;
	virtual void  CountStart() = 0;
	virtual float CountEnd() = 0;
};

extern "C" COMMONLIB_API void CreateTimeObject(ITimeObject** ppTimeObject);
extern "C" COMMONLIB_API void DeleteTimeObject(ITimeObject* pTimeObject);
