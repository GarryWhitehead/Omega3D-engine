#include "ThreadPool.h"

namespace OmegaEngine
{

ThreadPool::ThreadPool(uint8_t numThreads)
{
	for (uint8_t i = 0; i < numThreads; ++i)
	{
		threads.emplace_back(std::thread([this, i]() { this->worker(i); }));
	}
}

ThreadPool::~ThreadPool()
{
	isComplete = true;
	cv_task.notify_all();
	for (auto &thread : threads)
	{
		thread.join();
	}
}

void ThreadPool::worker(uint32_t thread_id)
{
	std::function<void()> func;
	while (!isComplete)
	{

		bool workerReady = false;
		{
			std::unique_lock<std::mutex> lock(mut);
			cv_task.wait(lock, [this]() { return isComplete || !tasks.empty(); });

			if (isComplete)
			{
				break;
			}

			if (!tasks.empty())
			{
				func = tasks.front();
				tasks.pop();
				taskCount++;
				workerReady = true;
			}
		}

		if (workerReady)
		{

			func();
			printf("Function executed on thread: %i\n", thread_id);
			std::unique_lock<std::mutex> lock(mut);
			--taskCount;
			cv_finished.notify_one();
		}
	}
}

void ThreadPool::submitTask(std::function<void()> func)
{
	{
		std::lock_guard<std::mutex> guard(mut);
		tasks.push(std::move(func));
	}
	cv_task.notify_one();
}

bool ThreadPool::isFinished()
{
	std::lock_guard<std::mutex> guard(mut);
	return taskCount == 0 && tasks.empty();
}

void ThreadPool::waitForAll()
{
	//for (auto& thread : threads) {
	std::unique_lock<std::mutex> lock(mut);
	cv_finished.wait(lock, [this]() { return tasks.empty(); });
	//}
}

void ThreadPool::stopThread()
{
	isComplete = true;
	cv_task.notify_all();
}

} // namespace OmegaEngine
