/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ThreadPool.h"

namespace OmegaEngine
{

ThreadPool::ThreadPool(const uint32_t numThreads)
{
	if (!numThreads)
	{
		// one less thread as we are on the main one.
		threadCount = std::thread::hardware_concurrency() - 1;

		// Best warn if there are no other threads, performace will be hit!
		if (!threadCount)
		{
			printf("No extra threads were found for the current hardware setup.");
		}
	}
	else
	{
		threadCount = numThreads;
	}

	threads.resize(threadCount);
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
