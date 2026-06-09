#pragma once
#include "Interface.h"
#include <Windows.h>

class CTimeObject : public ITimeObject
{
public:
	void  Initialize() override;
	void  CountStart() override;
	float CountEnd() override;

protected:
	LARGE_INTEGER m_frequency;
	LARGE_INTEGER m_start;
	LARGE_INTEGER m_end;
};

