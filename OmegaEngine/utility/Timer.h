#pragma once
#include <chrono>

class Timer
{

public:
	using Time = std::chrono::high_resolution_clock::time_point;
	using TimeMs = std::chrono::nanoseconds;

	Timer();
	~Timer();

	void startTimer();
	void pasueTimer();

	TimeMs getTimeElapsed(bool reset);

private:
	Time current;

	bool isRunning = false;
};
