#pragma once

#include "Utility/GeneralUtil.h"
#include "Objects/ObjectTypes.h"

#include <memory>
#include <stdint.h>
#include <unordered_map>

namespace OmegaEngine
{
	struct ComponentBase;

	class Object
	{

	public:

		Object();
		~Object();

		// operator overloads
		bool operator==(const Object& obj) const;

		void add_child(Object& obj);
		Object& get_last_child();

		// helper functions
		uint64_t get_id() const;
		void set_id(const uint64_t _id);
		uint64_t get_parent() const;

		template <typename T>
		T& get_component()
		{
			uint32_t id = Util::TypeId<T>::id();
			assert(components.find(id) != components.end());
			
			T* derived = dynamic_cast<T*>(components[id].get());
			assert(derived != nullptr);
			return *derived;
		}

		template <typename T, typename... Args>
		void add_component(Args&&... args)
		{
			uint32_t id = Util::TypeId<T>::id();
			components[id] = std::make_unique<T>(std::forward<Args>(args)...);
		}

		template <typename T>
		void add_component()
		{
			uint32_t id = Util::TypeId<T>::id();
			components[id] = new T;
		}

		template <typename T>
		bool hasComponent()
		{
			uint32_t id = Util::TypeId<T>::id();
			if (components.find(id) == components.end()) {
				return false;
			}
			return true;
		}

		std::vector<Object>& get_children();

	private:

		uint64_t id = UINT64_MAX;
		uint64_t parent_id = UINT64_MAX;
		std::vector<Object> children;

		std::unordered_map<uint32_t, std::unique_ptr<ComponentBase> > components;

	};

}




