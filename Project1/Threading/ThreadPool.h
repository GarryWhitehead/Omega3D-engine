#pragma once
#include <chrono>
#include <vector>
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <functional>

class ThreadPool
{

public:

	ThreadPool();
	~ThreadPool();

private:

	std::vector<std::thread> threads;
	std::queue
};

