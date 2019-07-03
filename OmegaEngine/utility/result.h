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