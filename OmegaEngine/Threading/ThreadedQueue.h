#pragma once

#include <atomic>
#include <mutex>
#include <queue>

namespace OmegaEngine
{

template <typename T>
class ThreadedQueue
{
public:
	ThreadedQueue() = default;
	~ThreadedQueue()
	{
		terminate();
	}

	bool tryPop(T& value)
	{
		std::lock_guard<std::mutex> lock{ qeueMutex };
		if (values.empty() || finished)
		{
			return false;
		}

		value = std::move(values.front());
		values.pop();
		return true;
	}

	bool waitPop(T& value)
	{
		std::unique_lock<std::mutex> lock{ queueMutex };
		queueCondition.wait(lock, [this]() { return !values.empty() || finished; });

		if (finished)
		{
			return false;
		}

		value = std::move(values.front());
		values.pop();
		return true;
	}

	void push(T task)
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		values.push(std::move(task));
		queueCondition.notify_one();
	}

	void clear()
	{
		std::lock_guard<std::mutex> lock{ queueMutex };
		while (!values.empty())
		{
			values.pop();
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
	std::queue<T> values;
	std::condition_variable queueCondition;
	std::mutex queueMutex;
	std::atomic_bool finished{ false };
};

}    // namespace OmegaEngine
