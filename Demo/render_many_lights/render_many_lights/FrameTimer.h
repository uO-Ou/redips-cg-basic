#pragma once


class FrameTimer
{
public:
	static FrameTimer* instance();
private:
	static FrameTimer* m_instance;
private:
	static const int AVG_FRAME_COUNT = 10;

private:
	long long m_times[AVG_FRAME_COUNT];
	int m_curFrame = 0;
	int m_curTimesIndex;
	long long m_elapsedTime;
	float m_averageFrameRate;
	long long m_startTime;

	int m_nConstFrame;

	FrameTimer();

public:

	void setConstFrame(const int nFrame)
	{
		m_nConstFrame = nFrame;
	}

	void cancleConstFrame()
	{
		m_nConstFrame = -1;
	}



	void clear(const int nStartFrame = 0);


	void frameUpdate();

	float getFrameRate();
	long long getElapsedTime();

	static long long getSystemTime();

	int getCurFrame()
	{
		if (-1 != m_nConstFrame) return m_nConstFrame;
		return m_curFrame;
	}
};

#define F_CUR_FRAME       FrameTimer::instance()->getCurFrame()
#define F_UPDATE          FrameTimer::instance()->frameUpdate
#define F_FRAME_RATE      FrameTimer::instance()->getFrameRate()
#define F_TIME_PER_FRAME  FrameTimer::instance()->getElapsedTime()
#define F_CLEAR           FrameTimer::instance()->clear