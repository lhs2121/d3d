#include "pch.h"
#include "Interface.h"
#include "Timer.h"

void CreateTimer(ITimer** ppTimer)
{
	*ppTimer = new Timer();
}

void DeleteTimer(ITimer* timer)
{
	delete static_cast<Timer*>(timer);
}
