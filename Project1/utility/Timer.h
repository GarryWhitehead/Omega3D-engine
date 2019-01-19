#pragma once
#include <chrono>

class Timer
{

public:

	using Time = std::chrono::steady_clock::time_point;
	using TimeMs = std::chrono::milliseconds;

	Timer();
	~Timer();

	void start_timer();
	void pasue_timer();

	TimeMs get_time_elapsed(bool reset);

private:

	Time current;

	bool is_running = false;
};

