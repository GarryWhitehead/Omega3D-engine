#include "Timer.h"



Timer::Timer()
{
}


Timer::~Timer()
{
}

void Timer::startTimer()
{
	this->current = std::chrono::high_resolution_clock::now();
	this->isRunning = true;
	
}

void Timer::pasueTimer()
{
	this->isRunning = false;
}

Timer::TimeMs Timer::getTimeElapsed(bool reset)
{
	auto timeNow = std::chrono::high_resolution_clock::now(); 
	auto deltaTime = timeNow - this->current;

	if (reset) {
		this->current = timeNow;
	}

	return deltaTime;
}


