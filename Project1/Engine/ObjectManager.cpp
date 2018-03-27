#include "Engine/ObjectManager.h"
#include "Engine/Object.h"


ObjectManager::ObjectManager()
{
}


ObjectManager::~ObjectManager()
{
}

Object ObjectManager::CreateObject()
{
	uint32_t id = 0;
	if (m_freeIds.size() > MINIMUM_FREE_IDS) {

		id = m_freeIds.front();
		m_freeIds.pop_front();
		m_objects.push_back(id);
	}
	else {

		id = ++m_nextId;
		m_objects.push_back(id);
	}
	return Object(id);
}

void ObjectManager::DestroyObject(Object obj)
{
	uint32_t id = obj.GetId();
	m_objects.erase(m_objects.begin() + id);
	m_freeIds.push_front(id);
}

void ObjectManager::Serialise(Archiver *arch, std::vector<Object>& vec, Archiver::var_info &info)
{
	uint32_t vecSize = static_cast<uint32_t>(vec.size());
	arch->Serialise(vecSize, Archiver::var_info(info.name + ".size"));
	vec.resize(vecSize);

	for (uint32_t c = 0; c < vecSize; ++c) {

		vec[c].SerialiseObject(arch, vec[c], Archiver::var_info(info.name + "[" + std::to_string(c) + "]"));
		//m_objects.push_back(vec[c]);										// also add to the list of alive objects
	}
}