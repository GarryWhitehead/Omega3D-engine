#include "Timer.h"



Timer::Timer()
{
}


Timer::~Timer()
{
}

void Timer::start_timer()
{
	this->current = std::chrono::high_resolution_clock::now();
	this->getIsRunning = true;
	
}

void Timer::pasue_timer()
{
	this->getIsRunning = false;
}

Timer::TimeMs Timer::get_time_elapsed(bool reset)
{
	auto time_now = std::chrono::high_resolution_clock::now(); 
	auto delta_time = time_now - this->current;

	if (reset) {
		this->current = time_now;
	}

	return delta_time;
}


