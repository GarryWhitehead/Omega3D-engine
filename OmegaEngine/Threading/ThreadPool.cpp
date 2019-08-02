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
	workers.terminate();
	for (auto &thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

void ThreadPool::worker(uint32_t thread_id)
{
	while (!isComplete)
	{
		std::unique_ptr<Task> task;
		if (workers.waitPop(task))
		{
			task->executeTask();
		}
	}
}

} // namespace OmegaEngine
