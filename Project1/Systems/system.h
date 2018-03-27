#pragma once
#include <vector>
#include <assert.h>

class ComponentManager;
enum class ComponentManagerId;

class System
{
public:

	System();

	virtual ~System();

	virtual void Update() = 0;
	virtual void Destroy() = 0;

	void RegisterManager(ComponentManager *manager);

	template<typename T>
	T* GetRegisteredManager(ComponentManagerId id);

protected:

	std::vector<ComponentManager*> m_registeredManagers;
};

template<typename T>
T* System::GetRegisteredManager(ComponentManagerId id)
{
	assert(!m_registeredManagers.empty());
	uint32_t index = 0;
	for (auto& man : m_registeredManagers) {
		if (man->GetId() == id) {

			T* man = static_cast<T*>(m_registeredManagers[index]);
			assert(man != nullptr);
			return man;
		}
		else
			++index;
	}

	// id not found so report error
	*g_filelog << "Error locating registered manager within system. Id #" << (int)id << " not found.\n";
}
