#include "pch.h"
#include "Timer.h"

void Timer::Initialize()
{
	QueryPerformanceFrequency(&m_frequency);
}

void Timer::Reset()
{
	QueryPerformanceCounter(&m_start);
}

float Timer::GetElapsedSeconds()
{
	QueryPerformanceCounter(&m_end);
	float deltaTime = (m_end.QuadPart - m_start.QuadPart) / (float)m_frequency.QuadPart;
	return deltaTime;
}
