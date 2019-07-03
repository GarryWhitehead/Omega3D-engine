#pragma once
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <thread>
#include <vector>

namespace OmegaEngine
{

class ThreadPool
{

public:
	ThreadPool(uint8_t numThreads);
	~ThreadPool();

	void submitTask(std::function<void()> func);
	bool isFinished();
	void waitForAll();
	void stopThread();

private:
	void worker(uint32_t thread_id);

	std::vector<std::thread> threads;
	std::queue<std::function<void()>> tasks;

	// threading stuff
	std::mutex mut;
	std::atomic<bool> isComplete{ false };
	std::atomic<int> taskCount{ 0 };
	std::condition_variable_any cv_task;
	std::condition_variable cv_finished;
};

} // namespace OmegaEngine
