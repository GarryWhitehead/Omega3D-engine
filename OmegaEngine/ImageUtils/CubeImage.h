#pragma once

#include "Image2D.h"

namespace OmegaEngine
{

class CubeImage : public Image2D<float>
{
public:
	enum Face
	{
		pX,    // left
		nX,    // right
		pY,    // top
		nY,    // bottom
		pZ,    // back,
		nZ,    // front
		Size
	};

	Colour3 fetchBilinear(size_t x, size_t y, const Face face);

private:

	Image2D* faces[6];
}

}    // namespace OmegaEngine
