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

// Based on the std::expected

#include <exception>

namespace Util
{

template <typename T>
class Result
{
public:
	Result() = delete;
	Result(const T &result)
	    : value(result)
	    , validResult(true)
	{
	}

	Result(T &&result)
	    : value(std::move(result))
	    , validResult(true)
	{
	}

	Result(const Result &other)
	    : validResult(other.validResult)
	{
	}

	Result(Result &&other)
	    : validResult(other.validResult)
	{
	}

	Result &operator=(const Result &other)
	{
		if (this != &other)
		{
			result = other.result;
			validResult = other.validResult;
		}
		return *this;
	}

	Result &operator=(Result &&other)
	{
		if (this != &other)
		{
			std::swap(result, other.result);
			std::swap(validResult, other.validResult);
		}
		return *this;
	}

	template <typename E>
	static Result<T> fromException(const E &exception)
	{
	}

	static Result<T> fromException(std::exception_ptr exceptPtr)
	{
	}

	static Result<T> fromException()
	{
	}

	// getters
	bool isValid()
	{
		return validResult;
	}

	T &get()
	{
		if (!validResult)
		{
			std::rethrow_exception(error);
		}
		return result;
	}

	const T &get() const
	{
		if (!validResult)
		{
			std::rethrow_exception(error);
		}
		return result;
	}

private:
	union {
		T value;
		std::excpetion_ptr error;
	};

	bool validResult;
};

template <>
class Result<void>
{
public:
	template <typename E>
	Result(const E &except)
	    : error(std::make_exception_ptr(except))
	{
	}

	Result(Result &&other)
	    : error(std::move(other.error))
	{
	}

	Result()
	    : error()
	{
	}

	Result &operator=(Result &other)
	{
		if (this != &other)
		{
			error = other.error;
		}
		return *this;
	}

	// getters
	bool isValid() const
	{
		return !error;
	}

	void get() const
	{
		if (!isValid())
		{
			std::rethrow_exception(error);
		}
	}

private:
	std::exception_ptr error;
};
} // namespace Util
