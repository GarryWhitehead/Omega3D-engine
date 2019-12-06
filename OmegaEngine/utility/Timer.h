#pragma once

#include <chrono>

// shortened versions of the chrono lib definitions
using Time = std::chrono::high_resolution_clock::time_point;
using NanoSeconds = std::chrono::nanoseconds;
using Seconds = std::chrono::seconds;
using MilliSeconds = std::chrono::milliseconds;

namespace Util
{

template <typename TimeType>
class Timer
{

public:
	Timer(){};

	void startTimer()
	{
		current = std::chrono::duration_cast<TimeType>(std::chrono::high_resolution_clock::now());
		isRunning = true;
	}

	void pasueTimer()
	{
		isRunning = false;
	}

	void resetTime()
	{
		auto timeNow = std::chrono::duration_cast<TimeType>(std::chrono::high_resolution_clock::now());
		current = timeNow;
	}

	TimeType getTimeElapsed()
	{
		auto timeNow = std::chrono::duration_cast<TimeType>(std::chrono::high_resolution_clock::now());
		auto deltaTime = timeNow - current;

		return deltaTime;
	}

	static TimeType getCurrentTime()
	{
		return std::chrono::duration_cast<TimeType>(std::chrono::high_resolution_clock::now());
	}

private:

	Time current;

	bool isRunning = false;
};

}    // namespace Util
