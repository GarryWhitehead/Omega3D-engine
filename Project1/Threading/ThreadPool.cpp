#include "ThreadPool.h"

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
		con_var.wait(lock, [this]() {
			return isComplete || !workers.empty();
		});

		if (isComplete) {
			break;
		}

		if (!workers.empty()) {
			func = workers.front();
			workers.pop();
			workerCount++;
		}

		if (workerReady) {
			func();
			--workerCount;
		}
	}
}

void ThreadPool::submitWorker(std::function<void()> func)
{
	std::lock_guard<std::mutex> guard(mut);
	workers.push(func);
	con_var.notify_one();
}

bool ThreadPool::isFinished()
{
	std::lock_guard<std::mutex> guard(mut);
	return workerCount == 0 && workers.empty();
}

void ThreadPool::stopThread()
{
	isComplete = true;
	con_var.notify_all();
}