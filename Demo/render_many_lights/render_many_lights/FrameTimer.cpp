
#include <sys/timeb.h>
#include "FrameTimer.h"

FrameTimer* FrameTimer::m_instance = nullptr;

FrameTimer* FrameTimer::instance()
{
	if (nullptr == m_instance)
	{
		m_instance = new FrameTimer;
	}
	return m_instance;
}

FrameTimer::FrameTimer() :m_nConstFrame(-1)
{
}


void FrameTimer::clear(const int nStartFrame)
{
	m_curTimesIndex = 0;
	m_curFrame = nStartFrame;

}



void FrameTimer::frameUpdate()
{
	long long currentTime = getSystemTime();
	m_elapsedTime = currentTime - m_startTime;
	m_startTime = currentTime;


	m_times[m_curTimesIndex] = m_elapsedTime;
	if (++m_curTimesIndex == AVG_FRAME_COUNT) m_curTimesIndex = 0;

	long long sum = 0;
	for (int i = 0; i<AVG_FRAME_COUNT; i++) sum += m_times[i];

	m_elapsedTime = sum / AVG_FRAME_COUNT;
	if (m_elapsedTime>0.001) m_averageFrameRate = 1000.0f / (float)m_elapsedTime;
	else m_averageFrameRate = -1;

	++m_curFrame;

	//m_curFrame+=5;
}

float FrameTimer::getFrameRate()
{
	return m_averageFrameRate;
}

long long FrameTimer::getElapsedTime()
{
	return m_elapsedTime;
}

long long FrameTimer::getSystemTime()
{
	struct timeb tmp;
	ftime(&tmp);
	return (long long)tmp.time * 1000 + tmp.millitm;
}