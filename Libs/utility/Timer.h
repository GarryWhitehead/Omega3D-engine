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

#include <chrono>

// shortened versions of the chrono lib definitions
using Time = std::chrono::high_resolution_clock::time_point;
using NanoSeconds = std::chrono::nanoseconds;
using Seconds = std::chrono::seconds;
using MilliSeconds = std::chrono::milliseconds;

namespace Util
{

template <typename T>
struct is_chrono_duration
{
    static constexpr bool value = false;
};

template <typename Rep, typename Period>
struct is_chrono_duration<std::chrono::duration<Rep, Period>>
{
    static constexpr bool value = true;
};

template <typename TimeType = std::chrono::nanoseconds>
class Timer
{

public:
    static_assert(is_chrono_duration<TimeType>::value, "TimeType must be a std::chrono::duration");

    Timer() {};

    void startTimer()
    {
        current = std::chrono::duration_cast<TimeType>(std::chrono::high_resolution_clock::now());
        isRunning = true;
    }

    void pasueTimer()
    {
        isRunning = false;
    }

    void resetTime()
    {
        auto timeNow =
            std::chrono::duration_cast<TimeType>(std::chrono::high_resolution_clock::now());
        current = timeNow;
    }

    TimeType getTimeElapsed()
    {
        auto timeNow =
            std::chrono::duration_cast<TimeType>(std::chrono::high_resolution_clock::now());
        auto deltaTime = timeNow - current;

        return deltaTime;
    }

    static TimeType getCurrentTime()
    {
        auto timeNow = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<TimeType>(timeNow.time_since_epoch());
    }

private:
    Time current;

    bool isRunning = false;
};

} // namespace Util
