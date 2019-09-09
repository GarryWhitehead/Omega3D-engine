#pragma once

#include "ObjectInterface/ComponentTypes.h"
#include "Utility/GeneralUtil.h"

#include <stdint.h>
#include <unordered_map>

namespace OmegaEngine
{
struct ComponentBase;

class Object
{

public:
	Object();
	Object(const uint32_t _id);
	~Object();

	// operator overloads
	bool operator==(const Object &obj) const;

	Object &addChild(const uint32_t _id);

	// helper functions
	uint64_t getId() const;
	void setId(const uint64_t id);
	uint64_t getParent() const;

	template <typename T>
	T &getComponent()
	{
		uint32_t id = Util::TypeId<T>::id();
		assert(components.find(id) != components.end());

		T *derived = dynamic_cast<T *>(components[id]);
		assert(derived != nullptr);
		return *derived;
	}

	template <typename T, typename... Args>
	void addComponent(Args &&... args)
	{
		uint32_t id = Util::TypeId<T>::id();
		components[id] = new T(std::forward<Args>(args)...);
	}

	template <typename T>
	void addComponent()
	{
		uint32_t id = Util::TypeId<T>::id();
		components[id] = new T();
	}

	template <typename T>
	bool hasComponent()
	{
		uint32_t id = Util::TypeId<T>::id();
		if (components.find(id) == components.end())
		{
			return false;
		}
		return true;
	}

	bool hasChildren() const
	{
		return !children.empty();
	}

	std::vector<Object> &getChildren();

private:
	uint64_t id = UINT64_MAX;
	uint64_t parentId = UINT64_MAX;
	std::vector<Object> children;

	std::unordered_map<uint32_t, ComponentBase *> components;
};

} // namespace OmegaEngine
