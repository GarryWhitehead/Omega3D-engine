#pragma once
#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
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

template <typename T>
class ThreadedQueue
{
public:
	ThreadedQueue() = default;
	~ThreadedQueue()
	{
		terminate();
	}

	bool tryPop(T& task)
	{
		std::lock_guard<std::mutex> lock{ qeueMutex };
		if (tasks.empty() || finished)
		{
			return false;
		}

		task = std::move(tasks.front());
		tasks.pop();
		return true;
	}

	bool waitPop(T& task)
	{
		std::unique_lock<std::mutex> lock{ queueMutex };
		queueCondition.wait(lock, [this]() { return !tasks.empty() || finished; });

		if (finished)
		{
			return false;
		}

		task = std::move(tasks.front());
		tasks.pop();
		return true;
	}

	void pushTask(const T task)
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		tasks.push(std::move(task));
		queueCondition.notify_one();
	}

	void clearAll()
	{
		std::lock_guard<std::mutex> lock{ queueMutex };
		while (!tasks.empty())
		{
			tasks.pop();
		}
		queueCondition.notify_all();
	}

	void terminate()
	{
		std::lock_guard<std::mutex> lock{ queueMutex };
		finished = true;
		queueCondition.notify_all();
	}

private:
	std::queue<T> tasks;
	std::condition_variable queueCondition;
	std::mutex queueMutex;
	std::atomic_bool finished{ false };
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
		~ThreadTask() override = default;

		ThreadTask(const ThreadTask&) = delete;
		ThreadTask& operator=(const ThreadTask&) = delete;

		ThreadTask(ThreadTask&&) = default;
		ThreadTask& operator=(ThreadTask&&) = default;

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
	ThreadPool(uint8_t numThreads);
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
		workers.pushTask(std::make_unique<ThreadTask<PackagedTask>>(std::move(pTask)));

		return result;
	}

private:
	void worker(uint32_t thread_id);

	std::vector<std::thread> threads;
	ThreadedQueue<std::unique_ptr<Task>> workers;

	std::atomic_bool isComplete{ false };
};

}    // namespace OmegaEngine
