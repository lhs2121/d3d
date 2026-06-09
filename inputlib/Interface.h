#pragma once

#ifdef INPUTLIB_EXPORTS
#define INPUTLIB_API __declspec(dllexport)
#else
#define INPUTLIB_API __declspec(dllimport)
#endif

struct IEngine;

struct IInputObject
{
	virtual void Initailize() = 0;

	virtual void UpdateKeyStates() = 0;

	virtual bool IsDown(int _key, void* pUser) = 0;

	virtual bool IsPress(int _key, void* pUser) = 0;

	virtual bool IsUp(int _key, void* pUser) = 0;

	virtual bool IsFree(int _key, void* pUser) = 0 ;

	virtual void AddUser(void* pUser) = 0;
};

extern "C" INPUTLIB_API void CreateInputObject(IInputObject** ppTimeObject);

extern "C" INPUTLIB_API void DeleteInputObject(IInputObject* pTimeObject);
