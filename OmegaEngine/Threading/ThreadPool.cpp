#include "ThreadPool.h"


namespace OmegaEngine
{

ThreadPool::ThreadPool(const uint8_t numThreads)
{
	for (uint8_t i = 0; i < numThreads; ++i)
	{
		threads.emplace_back(&ThreadPool::worker, this);
	}
}

ThreadPool::~ThreadPool()
{
	isComplete = true;
	workers.terminate();
	for (auto& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
}

void ThreadPool::worker()
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

}    // namespace OmegaEngine
