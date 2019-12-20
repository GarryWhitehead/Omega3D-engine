#pragma once

#include "Scripting/LuaBinder.h"

#include "utility/CString.h"

#include "lua/lua.hpp"

#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace OmegaEngine
{

class EngineConfig
{
public:

	EngineConfig() = default;

	friend class ConfigParser;

	/**
	* @brief Trys to find the specified config name in the map and if successful returns the value stored.
	* If not successful, returns the default value which is specified by the user and inserts into the map the name and value.
	* Note: if the user mismathes the template type vs the lua type, then throws an exception
	*/
	template <typename T>
	T findOrInsert(std::string name, T defaultVal)
	{
		auto iter = configs.find(name);
		if (iter == configs.end())
		{
			if constexpr (std::is_same_v<T, float>)
			{
				configs.emplace(name, TypeDescriptor{ TypeDescriptor::Float, std::to_string(defaultVal) });
			}
			else if constexpr (std::is_same_v<T, int>)
			{
				configs.emplace(name, TypeDescriptor{ TypeDescriptor::Int, std::to_string(defaultVal) });
			}
			if constexpr (std::is_same_v<T, std::string>)
			{
				configs.emplace(name, TypeDescriptor{ TypeDescriptor::String, defaultVal });
			}
			else
			{
				throw std::runtime_error("Unsupported value template type used.");
			}
			return defaultVal;
		}

		TypeDescriptor::Type type = iter->second.type;
		switch (type)
		{
		case TypeDescriptor::Float:
		{
			if constexpr (std::is_same_v<T, float>)
			{
				return std::stof(iter->second.value);
			}
			break;
		}
		case TypeDescriptor::Int:
		{
			if constexpr (std::is_same_v<T, int>)
			{
				return std::stoi(iter->second.value);
			}
			break;
		}
		case TypeDescriptor::String:
		{
			if constexpr (std::is_same_v<T, std::string>)
			{
				return iter->second.value;
			}
			break;
		}
        case TypeDescriptor::Invalid:
                break;
		}
		// we shouldn't be here if there was no error. Not much to do but assert or throw!
		throw std::runtime_error("Mismatched types between lua input and user type.");
	}

	/**
	* @brief Clears all config settings from the map
	*/
	void clear();

	/**
	* @brief 
	*/
	bool hasConfig(std::string name) const;
	bool empty() const;
	TypeDescriptor::Type getType(std::string name) const;

private:
	// first = name; second = descriptor
	std::unordered_map<std::string, TypeDescriptor> configs;
};

class ConfigParser
{
public:
	ConfigParser() = delete;
	ConfigParser(LuaBinder& binder, EngineConfig& config);

	bool parse(Util::String filename);

private:
	LuaBinder& binder;
	EngineConfig& config;
};

}    // namespace OmegaEngine
