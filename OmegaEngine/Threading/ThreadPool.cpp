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
			cv_task.wait(lock, [this]() {
				return isComplete || !tasks.empty();
			});

			if (isComplete) {
				break;
			}

			if (!tasks.empty()) {
				func = tasks.front();
				tasks.pop();
				workerReady = true;
				taskCount++;
			}

			if (workerReady) {
				func();
				--taskCount;
				cv_finished.notify_one();
			}
		}
	}

	void ThreadPool::submitTask(std::function<void()> func)
	{
		std::lock_guard<std::mutex> guard(mut);
		tasks.push(func);
		cv_task.notify_one();
	}

	bool ThreadPool::isFinished()
	{
		std::lock_guard<std::mutex> guard(mut);
		return taskCount == 0 && tasks.empty();
	}

	void ThreadPool::wait_for_all()
	{
		std::unique_lock<std::mutex> lock(mut);
		cv_finished.wait(lock, [this]() { return taskCount == 0 && tasks.empty(); });
	}

	void ThreadPool::stopThread()
	{
		isComplete = true;
		cv_task.notify_all();
	}

}