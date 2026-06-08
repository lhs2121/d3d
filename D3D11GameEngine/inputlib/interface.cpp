#include "pch.h"
#include "Interface.h"
#include "InputObject.h"

void CreateInputObject(IInputObject** ppTimeObject)
{
	*ppTimeObject = new CInputObject();
}

void DeleteInputObject(IInputObject* pTimeObject)
{
	CInputObject* pCast = (CInputObject*)pTimeObject;
	delete pCast;
}
