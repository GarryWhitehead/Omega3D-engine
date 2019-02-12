#pragma once
#include <future>

namespace ThreadUtil
{
	
	// A simple wrapper to ensure that async really is asynchronous - taken from Scott Myers Effective Modern C++
	template<typename T, typename... Ts>
	inline auto forceAsync(T&&, Ts&&... args)
	{
		return std::async(std::launch::async, std::forward<F>(f), std::forward<Ts>(args)...);
	}

	// a wrapper to check whther task has finished
	template<typename T>
	bool isTaskComplete(const std::future<T> &fut)
	{
		return fut.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
	}
}

