#pragma once

#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"
#include "Utility/GeneralUtil.h"
#include "Utility/Logger.h"

#include <memory>
#include <unordered_map>

namespace OmegaEngine
{

// forward declearations
class Object;
class ObjectManager;

class ComponentInterface
{

public:
	ComponentInterface();
	~ComponentInterface();

	void addObjectToUpdateQueue(Object *object);
	void updateManagersRecursively(Object *object);
	void updateManagersFromQueue();

	void update(double time, double dt, std::unique_ptr<ObjectManager> &objectManager);

	template <typename T, typename... Args>
	void registerManager(Args &&... args)
	{
		uint32_t managerId = Util::TypeId<T>::id();
		if (managers.find(managerId) != managers.end())
		{
			LOGGER_ERROR("Fatal error! Duplicated manager ids!");
		}

		managers[managerId] = std::make_unique<T>(std::forward<Args>(args)...);
		assert(managers[managerId] != nullptr);
		managers[managerId]->setId(managerId);
	}

	template <typename T>
	T &getManager()
	{
		uint32_t managerId = Util::TypeId<T>::id();
		if (managers.find(managerId) == managers.end())
		{
			// something is fundamentally wrong if this occurs
			LOGGER_ERROR("Unable to find manager in component interface. Unable to continue.");
		}

		T *derived = dynamic_cast<T *>(managers[managerId].get());
		assert(derived != nullptr);
		return *derived;
	}

	template <typename T>
	void removeManager()
	{
		uint32_t managerId = Util::TypeId<T>::id();
		if (managers.find(managerId) == manager.end())
		{
			// continue for now but if we see this then somethings wrong
			LOGGER_INFO("Unable to erase manager from component interface.");
		}
		else
		{
			managers.erase(managerId);
		}
	}

	template <typename T>
	bool hasManager()
	{
		uint32_t managerId = Util::event_type_id<T>();
		if (managers.find(managerId) != manager.end())
		{
			return true;
		}
		return false;
	}

protected:
	std::vector<Object *> objectUpdateQueue;

	std::unordered_map<uint32_t, std::unique_ptr<ManagerBase>> managers;
};

} // namespace OmegaEngine
