#pragma once
#include <vector>
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <functional>

namespace OmegaEngine
{

	class ThreadPool
	{

	public:

		ThreadPool(uint8_t numThreads);
		~ThreadPool();

		void submitTask(std::function<void()> func);
		bool isFinished();
		void stopThread();

	private:

		void worker();

		std::vector<std::thread> threads;
		std::queue<std::function<void()> > tasks;

		// threading stuff
		std::mutex mut;
		std::atomic<bool> isComplete{ false };
		std::atomic<int> taskCount{ 0 };
		std::condition_variable_any con_var;

	};

}
