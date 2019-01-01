#pragma once

#include <unordered_map>

namespace OmegaEngine
{

	// forward declearations
	class Object;

	class ComponentManagerBase
	{
	public:

		// abstract functions
		virtual void Update() = 0;

		void addObject(Object& obj, uint64_t index)
		{
			objects[obj] = index;
		}

		bool findObject(Object& obj, uint64_t& index)
		{
			auto iter = objects.find(obj);
			if (iter == objects.end()) {
				return false;
			}
			index = iter->second;
			return true;
		}

	protected:

		struct HashObject
		{
			size_t operator()(const Object& obj) const
			{
				return(std::hash<uint64_t>()(obj.getId()));
			}
		};

		std::unordered_map<Object, uint64_t> objects;

	};

}