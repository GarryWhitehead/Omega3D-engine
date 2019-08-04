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
	explicit ThreadPool(const uint8_t numThreads);
	~ThreadPool();

	ThreadPool(const ThreadPool&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;

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

}    // namespace OmegaEngine
