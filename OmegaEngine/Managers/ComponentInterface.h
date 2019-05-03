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

	enum class ManagerType
	{
		None,
		Mesh,
		Material,
		Texture,
		Camera,
		Light,
		Transform
	};

	class ComponentInterface
	{

	public:

		ComponentInterface();
		~ComponentInterface();

		void update_managers(double time, double dt, std::unique_ptr<ObjectManager>& obj_manager);
		
		template<typename T, typename... Args>
		void registerManager(Args&&... args)
		{
			uint32_t man_id = Util::TypeId<T>::id();
			if (managers.find(man_id) != managers.end()) 
			{
				LOGGER_ERROR("Fatal error! Duplicated manager ids!");
			}

			managers[man_id] = std::make_unique<T>(std::forward<Args>(args)...);
			assert(managers[man_id] != nullptr);
			managers[man_id]->set_id(man_id);
		}

		template <typename T>
		T& getManager()
		{
			uint32_t man_id = Util::TypeId<T>::id();
			if (managers.find(man_id) != managers.end()) 
			{
				T* derived = dynamic_cast<T*>(managers[man_id].get());
				assert(derived != nullptr);
				return *derived;
			}
			// something is fundamentally wrong if this occurs
			throw std::out_of_range("Unable to find manager in component interface. Unable to continue.");
		}

		template <typename T>
		void removeManager()
		{
			uint32_t man_id = Util::TypeId<T>::id();
			if (managers.find(man_id) != manager.end()) 
			{
				managers.erase(man_id);
			}
			// continue for now but if we see this then somethings wrong
			LOGGER_INFO("Unable to erase manager from component interface.");
		}

		template <typename T>
		bool hasManager()
		{
			uint32_t man_id = Util::event_type_id<T>();
			if (managers.find(man_id) != manager.end()) 
			{
				return true;
			}
			return false;
		}

	protected:

		std::unordered_map<uint32_t, std::unique_ptr<ManagerBase> > managers;
	};

}

