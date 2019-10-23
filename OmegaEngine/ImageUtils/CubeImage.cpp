#include "CubeImage.h"

namespace OmegaEngine
{
	
Colour3<float> CubeImage::fetchBilinear(size_t x, size_t y, Image2D* data)
{
	constexpr size_t x0 = x;
	constexpr size_t y0 = y;
	constexpr size_t x1 = x0 + 1;
	constexpr size_t y1 = y0 + 1;

	constexpr float u = x0;
	constexpr float v = y0;

	// get the four texels - x0-x1 and y0-y1
	Colour3<float> t0 = data->getTexel(x0, y0);
	Colour3<float> t1 = data->getTexel(x1, y0);
	Colour3<float> t2 = data->getTexel(x0, y1);
	Colour3<float> t3 = data->getTexel(x1, y1);

	return (1 - u) * (1 - v) * c0 + (u * 1 - v) * c1 + (1 - u * v) * c2 + u * v * c3;
}

}