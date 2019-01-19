#include "ThreadPool.h"

namespace OmegaEngine
{

	ThreadPool::ThreadPool(uint8_t numThreads)
	{
		for (uint8_t i = 0; i < numThreads; ++i) {
			threads.emplace_back(std::thread([this]() {
				worker();
			}));
		}
	}


	ThreadPool::~ThreadPool()
	{
		isComplete = true;
		for (auto& thread : threads) {
			thread.join();
		}
	}

	void ThreadPool::worker()
	{
		std::function<void()> func;
		while (!isComplete) {

			bool workerReady = false;

			std::unique_lock<std::mutex> lock(mut);
			con_var.wait(lock, [this]() {
				return isComplete || !tasks.empty();
			});

			if (isComplete) {
				break;
			}

			if (!tasks.empty()) {
				func = tasks.front();
				tasks.pop();
				taskCount++;
			}

			if (workerReady) {
				func();
				--taskCount;
			}
		}
	}

	void ThreadPool::submitTask(std::function<void()> func)
	{
		std::lock_guard<std::mutex> guard(mut);
		tasks.push(func);
		con_var.notify_one();
	}

	bool ThreadPool::isFinished()
	{
		std::lock_guard<std::mutex> guard(mut);
		return taskCount == 0 && tasks.empty();
	}

	void ThreadPool::wait_for_all()
	{
		while (!isFinished()) {

		}
	}

	void ThreadPool::stopThread()
	{
		isComplete = true;
		con_var.notify_all();
	}

}