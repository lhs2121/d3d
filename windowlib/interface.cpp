#include "pch.h"
#include "Interface.h"
#include "Window.h"

void CreateWindowInstance(IWindow** ppWindow)
{
	*ppWindow = new Window;
}

void DeleteWindowInstance(IWindow* window)
{
	delete static_cast<Window*>(window);
}
