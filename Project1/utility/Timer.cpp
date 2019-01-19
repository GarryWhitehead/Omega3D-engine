#include "Timer.h"



Timer::Timer()
{
}


Timer::~Timer()
{
}

void Timer::start_timer()
{
	this->current = std::chrono::steady_clock::now();
	this->is_running = true;
	
}

void Timer::pasue_timer()
{
	this->is_running = false;
}

Timer::TimeMs Timer::get_time_elapsed(bool reset)
{
	auto time_now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_now - this->current);

	if (reset) {
		this->current = time_now;
	}

	return elapsed;
}


