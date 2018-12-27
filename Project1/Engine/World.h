#pragma once
#include <string>
#include <vector>

namespace OmegaEngine
{
	// forward declerartions
	class ComponentSystem;

	enum class Managers
	{
		OE_MANAGERS_MESH	  	 = 1 << 0,
		OE_MANAGERS_LIGHT		 = 1 << 1,
		OE_MANAGERS_TRANSFORM	 = 1 << 2,
		OE_MANAGERS_ANIMATION	 = 1 << 3,
		OE_MANAGERS_PHYSICS		 = 1 << 4,
		OE_MANAGERS_COLLISION	 = 1 << 5,
		OE_MANAGERS_ALL			 = 1 << 6
	};

	// bitwise overload so casts aren't needed
	inline bool operator& (Managers a, Managers b)
	{
		return static_cast<int>(a) & static_cast<int>(b);
	}

	inline Managers operator| (Managers a, Managers b)
	{
		return static_cast<Managers>(static_cast<int>(a) | static_cast<int>(b));
	}

	class World
	{
	public:

		World(Managers managers);
		~World();

		bool create(const char* filename);
		void update();

	private:

		std::unique_ptr<ComponentSystem> compSystem;

	};

}
