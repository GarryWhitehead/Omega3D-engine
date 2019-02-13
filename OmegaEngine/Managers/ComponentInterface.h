#pragma once

#include "Utility/GeneralUtil.h"
#include "OEMaths/OEMaths.h"
#include "Managers/ManagerBase.h"
#include "Utility/Logger.h"

#include <unordered_map>
#include <memory>

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

		void update_managers(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager);
		
		template<typename T, typename... Args>
		void registerManager(Args&&... args)
		{
			uint32_t man_id = Util::event_type_id<T>();
			managers[man_id] = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
			managers[man_id]->set_id(man_id);
		}

		template <typename T>
		T& getManager()
		{
			uint32_t man_id = Util::event_type_id<T>();
			if (managers.find(man_id) != managers.end()) {
				T* derived = dynamic_cast<T*>(managers[man_id].get());
				return *derived;
			}
			// something is fundamentally wrong if this occurs
			throw std::out_of_range("Unable to find manager in component interface. Unable to continue.");
		}

		template <typename T>
		void removeManager()
		{
			uint32_t man_id = Util::event_type_id<T>();
			if (managers.find(man_id) != manager.end()) {
				managers.erase(man_id);
			}
			// continue for now but if we see this then somethings wrong
			LOGGER_ERROR("Unable to erase manager from component interface.");
		}

		template <typename T>
		bool hasManager()
		{
			uint32_t man_id = Util::event_type_id<T>();
			if (managers.find(man_id) != manager.end()) {
				return true;
			}
			return false;
		}

	protected:

		std::unordered_map<uint32_t, std::unique_ptr<ManagerBase> > managers;
	};

}
