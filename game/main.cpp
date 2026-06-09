#include "pch.h"
#include <Windows.h>
#include <inputlib/Interface.h>
#include <commonlib/Interface.h>
#include <rendererlib/Interface.h>
#include <windowlib/Interface.h>
#include "GameEngine.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	//Debug::CrtSetBreakAlloc(406);
	Debug::CrtSetDbgFlag();

	GameEngine engine;
	engine.Start("LegoEngine <DX11>", 50, 50, 1366, 789, hInstance);
}
