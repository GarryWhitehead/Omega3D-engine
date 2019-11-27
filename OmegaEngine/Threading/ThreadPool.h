#pragma once

#include "Threading/ThreadedQueue.h"

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

namespace OmegaEngine
{

template <typename T>
class TaskFuture
{
public:
	~TaskFuture()
	{
		if (fut.valid())
		{
			fut.get();
		}
	}

	TaskFuture(std::future<T>&& _fut)
	    : fut(std::move(_fut))
	{
	}

	// no copying allowed
	TaskFuture(const TaskFuture&) = delete;
	TaskFuture& operator=(const TaskFuture&) = delete;

	TaskFuture(TaskFuture&&) = default;
	TaskFuture& operator=(TaskFuture&&) = default;

	auto get()
	{
		return fut.get();
	}

private:
	std::future<T> fut;
};

class ThreadPool
{

private:
	class Task
	{
	public:
		Task() = default;
		virtual ~Task() = default;

		// no copying allowed
		Task(const Task&) = delete;
		Task& operator=(const Task&) = delete;

		Task(Task&&) = default;
		Task& operator=(Task&&) = default;

		// pure abstract function
		virtual void executeTask() = 0;
	};


	template <typename ThreadedFunc>
	class ThreadTask : public Task
	{
	public:
		~ThreadTask() = default;

		ThreadTask(const ThreadTask&) = delete;
		ThreadTask& operator=(const ThreadTask&) = delete;

		ThreadTask(ThreadTask&&) = default;
		ThreadTask& operator=(ThreadTask&&) = default;

		ThreadTask() = default;

		ThreadTask(ThreadedFunc&& _func)
		    : func(std::move(_func))
		{
		}

		void executeTask() override
		{
			func();
		}

	private:
		ThreadedFunc func;
	};

public:
	/**
	* Constructor : if number of threads is zero, then will create as many threads as the hardware will allow
	*/
	explicit ThreadPool(const uint8_t numThreads = 0);
	~ThreadPool();

	// not copyable
	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

	template <typename T>
	struct ThreadState
	{
		std::future<T> fut;
	};

	/**
	* Submits a task to the queue. 
	* Returns a std::future, this can be used for waiting for that thread to finish.
	*/
	template <typename ThreadFunc, typename... Args>
	auto submitTask(ThreadFunc&& func, Args&&... args)
	{
		auto task = std::bind(std::forward<ThreadFunc>(func), std::forward<Args>(args)...);

		using ResultType = std::result_of_t<decltype(task)()>;
		using PackagedTask = std::packaged_task<ResultType()>;

		PackagedTask pTask{ std::move(task) };
		TaskFuture<ResultType> result{ pTask.get_future() };
		workers.push(std::make_unique<ThreadTask<PackagedTask>>(std::move(pTask)));

		return std::move(result);
	}

private:
	void worker();

	std::vector<std::thread> threads;
	ThreadedQueue<std::unique_ptr<Task>> workers;


	std::atomic_bool isComplete{ false };
};

/**
* Splits a user defined total workload into the number of tasks possible with the threads 
* avialable, and executes each chunk of work on those threads. 
* If number of threads isn't defined, then will use all available threads
*/
template <typename Func>
class ThreadTaskSplitter
{
public:
	ThreadTaskSplitter(size_t start, size_t dataSize, Func func, uint8_t numThreads = 0)
	    : start(start)
	    , dataSize(dataSize)
	    ,
	    , func(func)
	    , pool(std::make_unique<ThreadPool>(numThreads))
	{
	}

	~ThreadTaskSplitter() = default;

	// not copyable
	ThreadTaskSplitter(const ThreadTaskSplitter&) = delete;
	ThreadTaskSplitter& operator=(ThreadTaskSplitter&) = delete;

	void run()
	{
		// calculate the data size each thread will process
		uint8_t numThreads = pool.getThreadCount();
		size_t end = start + dataSize;

		size_t chunkSize = 0;
		size_t processed = 0;

		// if the data size is smaller than the number of threads,
		// adjust the number of threads used
		if (dataSize < numThreads)
		{
			numThreads -= dataSize % numThreads;
		}
		size_t chunkSize = dataSize / numThreads;

		// submit the task with the calaculated chunk size
		// if there is only one thread available, then submit in all data in that thread
		// otherwise split....
		size_t pos = start;
		if (numTheads >= 2)
		{

			for (uint8_t i = 0; i < numThreads - 1; ++i)
			{
				pool->submitTask(func, pos, chunkSize);
				pos += i * chunkSize;
				processed += chunkSize;
			}
		}

		// finish by adding remaining data to process to the last thread
		pool->submitTask(func, pos, end - processed);
		
	}

private:
	std::make_unique<ThreadPool> pool;

	size_t start;
	size_t dataSize;
	Func func;
};

}    // namespace OmegaEngine
