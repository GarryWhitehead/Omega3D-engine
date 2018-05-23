#pragma once
#include "utility/serialisation.h"
#include <vector>

class World;
class ObjectManager;
enum class SystemId;

enum class ComponentManagerId
{
	CM_PHYSICS_ID,
	CM_TRANSFORM_ID,
	CM_MESH_ID,
	CM_ANIMATION_ID,
	CM_LIGHT_ID
};

class ComponentManager
{

public:

	ComponentManager(ComponentManagerId id);
	~ComponentManager();

	virtual void Init(World *world, ObjectManager *manager) = 0;
	virtual void Update() = 0;
	virtual void Destroy() = 0;

	// Links this componant manager with another - for instance - the transform manager needs to know if any model positions have chnaged
	// since the last update and if this is the case, obatin the updated data and translate the posiitons 
	void RegisterWithManager(ComponentManagerId dstId);
	void RegisterManager(ComponentManager *manager);

	template<typename T>
	T* GetRegisteredManager(ComponentManagerId id);
	bool HasRegisteredManager(ComponentManagerId id);

	ComponentManagerId GetId() const { return m_id; }

	void Serialise(Archiver *arch, ComponentManager& comp, const Archiver::var_info& info);

protected:

	ComponentManagerId m_id;

	// all component managers have a link with the world that created them
	World *p_world;

	// and the object production line
	ObjectManager *p_objectManager;

	// other managers registered with this particular manager
	std::vector<ComponentManager*> m_registeredManagers;

	virtual void pack_unpack(Archiver *arch, const Archiver::var_info& info);
};

template<typename T>
T* ComponentManager::GetRegisteredManager(ComponentManagerId id)
{
	assert(!m_registeredManagers.empty());
	uint32_t index = 0;
	for (auto& man : m_registeredManagers) {
		if (man->m_id == id)
			break;
		else
			++index;
	}

	T* man = static_cast<T*>(m_registeredManagers[index]);
	assert(man != nullptr);
	return man;
}

// derived serialisation class required for component derived classes

template <typename T>
class ArchivableComponentManager : public ComponentManager
{
public:

	ArchivableComponentManager(T& derived, ComponentManagerId id) :
		m_derived(derived),
		ComponentManager(id)
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
