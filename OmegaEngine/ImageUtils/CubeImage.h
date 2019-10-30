#pragma once

#include "Image2D.h"

namespace OmegaEngine
{

template <typename T>
class CubeImage : public Image2D<T>
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

	Colour3 fetchBilinear(size_t x, size_t y, Image2D* data)
	{
		constexpr size_t x0 = x;
		constexpr size_t y0 = y;
		constexpr size_t x1 = x0 + 1;
		constexpr size_t y1 = y0 + 1;

		constexpr float u = x0;
		constexpr float v = y0;

		// get the four texels - x0-x1 and y0-y1
		Colour3 t0 = data->getTexel(x0, y0);
		Colour3 t1 = data->getTexel(x1, y0);
		Colour3 t2 = data->getTexel(x0, y1);
		Colour3 t3 = data->getTexel(x1, y1);

		return (1 - u) * (1 - v) * t0 + (u * 1 - v) * t1 + (1 - u * v) * t2 + u * v * t3;
	}

	Colour3 fetchTrilinear(size_t x, size_t y, Image2D* mip1, Image2D* mip2)
	{
	}

private:
	Image2D* faces[6];
}

}    // namespace OmegaEngine
