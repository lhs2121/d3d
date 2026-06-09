#include "pch.h"
#include "Interface.h"
#include "TimeObject.h"

void CreateTimeObject(ITimeObject** ppTimeObject)
{
	*ppTimeObject = new CTimeObject();
}

void DeleteTimeObject(ITimeObject* pTimeObject)
{
	CTimeObject* pCast =(CTimeObject*)pTimeObject;
	delete pCast;
}
