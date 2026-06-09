#include "pch.h"
#include "TimeObject.h"

void CTimeObject::Initialize()
{
	QueryPerformanceFrequency(&m_frequency);
}

void CTimeObject::CountStart()
{
	QueryPerformanceCounter(&m_start);
}

float CTimeObject::CountEnd()
{
	QueryPerformanceCounter(&m_end);
	float deltaTime = (m_end.QuadPart - m_start.QuadPart) / (float)m_frequency.QuadPart;
	return deltaTime;
}
