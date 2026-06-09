#pragma once
#include "Interface.h"
#include <Windows.h>

class Timer : public ITimer
{
public:
	void  Initialize() override;
	void  Reset() override;
	float GetElapsedSeconds() override;

protected:
	LARGE_INTEGER m_frequency;
	LARGE_INTEGER m_start;
	LARGE_INTEGER m_end;
};

