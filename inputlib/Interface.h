#pragma once

#ifdef INPUTLIB_EXPORTS
#define INPUTLIB_API __declspec(dllexport)
#else
#define INPUTLIB_API __declspec(dllimport)
#endif

struct IGameLoop;

struct IInput
{
	virtual void Initialize() = 0;

	virtual void Update() = 0;

	virtual bool IsDown(int _key, void* pUser) = 0;

	virtual bool IsPressed(int _key, void* pUser) = 0;

	virtual bool IsReleased(int _key, void* pUser) = 0;

	virtual bool IsFree(int _key, void* pUser) = 0 ;

	virtual void AddUser(void* pUser) = 0;
};

extern "C" INPUTLIB_API void CreateInput(IInput** ppInput);

extern "C" INPUTLIB_API void DeleteInput(IInput* input);
