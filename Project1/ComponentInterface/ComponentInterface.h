#pragma once

#include "Utility/GeneralUtil.h"
#include "OEMaths/OEMaths.h"

#include <unordered_map>
#include <memory>

namespace OmegaEngine
{

	// forward declearations
	class Object;
	class ComponentBase;

	class ComponentInterface
	{

	public:

		ComponentInterface();
		~ComponentInterface();

		template<typename T, typename... Args>
		void registerManager()
		{
			std::unique_ptr<T> temp = std::make_unique<T>(Args);
			assert(temp != nullptr);
			uint32_t man_id = Util::event_type_id<T>();
			managers.insert(std::make_pair(man_id, std::move(temp));
			managers[man_id]->id = man_id;
		}

		template <typename T>
		std::unique_ptr<T>& getManager()
		{
			uint32_t man_id = Util::event_type_id<T>();
			if (managers.find(man_id) != manager.end()) {
				return dynamic_cast<T&>(managers[man_id]);
			}
		}

		template <typename T>
		void removeManager()
		{
			uint32_t man_id = Util::event_type_id<T>();
			if (managers.find(man_id) != manager.end()) {
				managers.erase(man_id);
			}
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

		template <typename T>
		void addComponentToObject(Object& obj)
		{
			uint32_t man_id = Util::event_type_id<T>();
			if (managers.find(man_id) == manager.end()) {
				return;
			}
			managers[man_id]->addObject(obj);
		}

	protected:

		std::unordered_map<uint32_t, std::unique_ptr<ComponentBase> > managers;
	};

}

