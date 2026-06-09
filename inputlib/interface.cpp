#include "pch.h"
#include "Interface.h"
#include "Input.h"

void CreateInput(IInput** ppInput)
{
	*ppInput = new Input();
}

void DeleteInput(IInput* input)
{
	delete static_cast<Input*>(input);
}
