#pragma once

#include <chrono>

// shortened versions of the chrono lib definitions
using Time = std::chrono::high_resolution_clock::time_point;
using NanoSeconds = std::chrono::nanoseconds;
using Seconds = std::chrono::seconds;
using MilliSeconds = std::chrono::milliseconds;

namespace Util
{

template <typename T>
struct is_chrono_duration
{
    static constexpr bool value = false;
};

template <typename Rep, typename Period>
struct is_chrono_duration<std::chrono::duration<Rep, Period>>
{
    static constexpr bool value = true;
};

template <typename TimeType = std::chrono::nanoseconds>
class Timer
{

public:
    
    static_assert(is_chrono_duration<TimeType>::value, "TimeType must be a std::chrono::duration");
    
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
        auto timeNow = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<TimeType>(timeNow.time_since_epoch());
	}

private:

	Time current;

	bool isRunning = false;
};

}    // namespace Util
