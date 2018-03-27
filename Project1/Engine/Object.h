#pragma once
//#include "utility/serialisation.h"
#include <stdint.h>
#include <unordered_map>
#include "utility/serialisation.h"

class Archiver;

class Object
{
public:

	Object();
	Object(const Object &obj);
	Object(uint32_t id);
	~Object();

	bool operator==(const Object& obj) const;

	// helper functions
	uint32_t GetId() const { return m_id; }
	bool IsAlive() const { return m_isAlive;  }

	void SerialiseObject(Archiver *arch, Object& obj, Archiver::var_info &info);

private:

	uint32_t m_id;
	bool m_isAlive;
};

struct HashGameObj
{
	size_t operator()(const Object& obj) const
	{
		return(std::hash<uint32_t>()(obj.GetId()));
	}
};


