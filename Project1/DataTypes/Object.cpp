#include "DataTypes/Object.h"
#include "Engine/engine.h"

Object::Object()
{

}

Object::Object(uint32_t id) :
	m_id(id),
	m_isAlive(true)
{
}

Object::Object(const Object &obj)
{
	m_id = obj.m_id;
	m_isAlive = obj.m_isAlive;
}

Object::~Object() 
{
}

bool Object::operator==(const Object& obj) const
{
	return m_id == obj.m_id;
}

void Object::SerialiseObject(Archiver *arch, Object& obj, Archiver::var_info &info)
{
	arch->Serialise(obj.m_id, Archiver::var_info(info.name + ".m_id"));
	arch->Serialise(obj.m_isAlive, Archiver::var_info(info.name + ".m_isAlive"));
}
