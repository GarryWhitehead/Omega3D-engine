#include "ThreadPool.h"

#include "utility/Logger.h"

namespace OmegaEngine
{

ThreadPool::ThreadPool(const uint8_t numThreads)
{
	uint8_t threadCount;
	if (!numThreads)
	{
		// one less thread as we are on the main one.
		threadCount = std::thread::hardware_concurrency() - 1;

		// Best warn if there are no other threads, performace will be hit!
		if (!threadCount)
		{
			LOGGER_INFO("No extra threads were found for the current hardware setup.");
		}
	}
	else
	{
		threadCount = numThreads;
	}

	threads.resize(numThreads);
	for (uint8_t i = 0; i < threadCount; ++i)
	{
		threads[i] = std::thread(&ThreadPool::worker, this);
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
