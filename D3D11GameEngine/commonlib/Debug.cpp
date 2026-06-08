#include "pch.h"
#include "Interface.h"
#include <crtdbg.h>
#include <cassert>

void Debug::CrtSetBreakAlloc(UINT Number)
{
	_CrtSetBreakAlloc(Number);
}

void Debug::CrtSetDbgFlag()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}

void Debug::MsgBoxAssert(const char* errorMsg)
{
	MessageBox(nullptr, errorMsg, "error", MB_OK);
	assert(0);
}
