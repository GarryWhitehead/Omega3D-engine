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
        std::lock_guard<std::mutex> lock {queueMutex};
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
        std::unique_lock<std::mutex> lock {queueMutex};
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
        std::lock_guard<std::mutex> lock {queueMutex};
        while (!values.empty())
        {
            values.pop();
        }
        queueCondition.notify_all();
    }

    void terminate()
    {
        std::lock_guard<std::mutex> lock {queueMutex};
        finished = true;
        queueCondition.notify_all();
    }

private:
    std::queue<T> values;
    std::condition_variable queueCondition;
    std::mutex queueMutex;
    std::atomic_bool finished {false};
};

} // namespace OmegaEngine
