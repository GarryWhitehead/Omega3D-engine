#pragma once
#include "utility/serialisation.h"
#include <vector>
#include <iostream>

class World;
class ObjectManager;
enum class SystemId;

class ComponentManager
{

public:

	ComponentManager();
	~ComponentManager();

	virtual void Init(World *world, ObjectManager *manager) = 0;
	virtual void Update() = 0;
	virtual void Destroy() = 0;

	// Links this componant manager with another - for instance - the transform manager needs to know if any model positions have chnaged
	// since the last update and if this is the case, obatin the updated data and translate the posiitons 
	
	void Serialise(Archiver *arch, ComponentManager& comp, const Archiver::var_info& info);

protected:

	// all component managers have a link with the world that created them
	World *p_world;

	// and the object production line
	ObjectManager *p_objectManager;

	virtual void pack_unpack(Archiver *arch, const Archiver::var_info& info);
};

// derived serialisation class required for component derived classes

template <typename T>
class ArchivableComponentManager : public ComponentManager
{
public:

	ArchivableComponentManager(T& derived) :
		m_derived(derived)
	{}

	~ArchivableComponentManager() {}

protected:

	void pack_unpack(Archiver *arch, const  Archiver::var_info& info)
	{
		m_derived.Serialise(arch, m_derived, info);
	}

private:

	T &m_derived;
};
