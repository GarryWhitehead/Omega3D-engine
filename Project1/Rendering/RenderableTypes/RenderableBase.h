#pragma once

namespace OmegaEngine
{

	enum class RenderTypes
	{
		Mesh,
		Skybox,
		Ocean,
		Terrain,
		Count
	};

	// abstract base class
	class RenderableBase
	{
	public:

		RenderableBase(RenderTypes t) :
			type(t)
		{
		}

		virtual ~RenderableBase() {}

		//virtual void render() = 0;

		RenderTypes get_type() const
		{
			return type;
		}

	protected:

		RenderTypes type;
	};

}