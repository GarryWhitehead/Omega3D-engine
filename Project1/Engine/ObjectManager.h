#pragma once
#include <vector>
#include <deque>
// The main purpose of the entity mnanager is to keep a track of all the entites that are alive

#include "Engine/Object.h"

class Archiver;

class ObjectManager
{
public:

	const int MINIMUM_FREE_IDS = 100;

	ObjectManager();
	~ObjectManager();

	Object CreateObject();
	void DestroyObject(Object obj);

	void Serialise(Archiver *arch, std::vector<Object>& vec, Archiver::var_info &info);

private:

	uint32_t m_nextId;

	std::vector<Object> m_objects;
	std::deque<uint32_t> m_freeIds;
};



