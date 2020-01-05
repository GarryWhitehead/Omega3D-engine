#pragma once

#include "Scripting/LuaBinder.h"

#include "utility/CString.h"

#include "OEMaths/OEMaths.h"

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
     * @brief Inserts a configuration parameter into the list with the specified value. Throws if the type of value isn't supported.
     */
    template <typename T>
    void insertItem(std::string name, T val)
    {
        if constexpr (std::is_same_v<T, float>)
        {
            configs.emplace(name, TypeDescriptor{ TypeDescriptor::Float, std::to_string(val) });
        }
        else if constexpr (std::is_same_v<T, int>)
        {
            configs.emplace(name, TypeDescriptor{ TypeDescriptor::Int, std::to_string(val) });
        }
        if constexpr (std::is_same_v<T, std::string>)
        {
            configs.emplace(name, TypeDescriptor{ TypeDescriptor::String, val });
        }
        else
        {
            throw std::runtime_error("Unsupported value template type used.");
        }
    }
    
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
            insertItem(name, defaultVal);
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
    
    void insertVec(std::string name, const float defaultVec[], uint32_t size);
    std::vector<float> buildVec(TypeDescriptor& decsr, uint8_t vecSize);
    
    // TODO: these could/should be probably templated at some point
    OEMaths::vec2f findOrInsertVec2(std::string name, const float defaultVec[]);
    OEMaths::vec3f findOrInsertVec3(std::string name, const float defaultVec[]);
    OEMaths::vec4f findOrInsertVec4(std::string name, const float defaultVec[]);
    
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
