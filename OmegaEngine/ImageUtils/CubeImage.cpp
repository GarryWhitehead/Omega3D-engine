#include "CubeImage.h"

namespace OmegaEngine
{

OEMaths::vec3f CubeImage::calculateNormal(float x, float y, const Face face, float dim)
{
	// convert to -1, 1 co-ord frame
	float x0 = x * (2.0f / dim) - 1.0f;
	float y0 = 1.0f - (y * (2.0f / dim));
	float len = OEMaths::length(OEMaths::vec3f{ x0, y0, 1.0f });
	float invlen = 1.0f / len;

	// create direction vector based on which face we are looking at
	OEMaths::vec3f dir;
	switch (face)
	{
	case Face::pX:
		dir = { 1.0f, y0, -x0 };
		break;
	case Face::nX:
		dir = { -1.0f, y0, x0 };
		break;
	case Face::pY:
		dir = { x0, 1.0f, -y0 };
		break;
	case Face::nY:
		dir = { x0, -1.0f, y0 };
		break;
	case Face::pZ:
		dir = { x0, y0, 1.0f };
		break;
	case Face::nZ:
		dir = { -x0, y0, -1.0f };
		break;
	}

	return dir / invlen;
}

CubeImage::FaceTexCoords CubeImage::calculateTexCoords(OEMaths::vec3f reflect)
{

	float rx = reflect.x;
	float ry = reflect.y;
	float rz = reflect.z;

	// keep track of which face the reflection vector refers to
	Face majorAxis;

	float absX = std::abs(rx);
	float absY = std::abs(ry);
	float absZ = std::abs(rz);

	if (absX > absY && absX > absZ)
	{
		if (rx <= 0)
		{
			majorAxis = Face::nX;
		}
		else
		{
			majorAxis = Face::pX;
		}
	}
	else if (absY > absX && absY > absZ)
	{
		if (ry <= 0)
		{
			majorAxis = Face::nY;
		}
		else
		{
			majorAxis = Face::pY;
		}
	}
	else
	{
		if (rz <= 0)
		{
			majorAxis = Face::nZ;
		}
		else
		{
			majorAxis = Face::pZ;
		}
	}

	float sc, tc, ma;

	switch (majorAxis)
	{

	case Face::pX:
	{

		sc = -rz;
		tc = -ry;
		ma = rx;
	}
	break;
	case Face::nX:
	{

		sc = rz;
		tc = -ry;
		ma = rx;
	}
	break;
	case Face::pY:
	{

		sc = -rx;
		tc = -rz;
		ma = ry;
	}
	break;
	case Face::nY:
	{

		sc = rx;
		tc = -rz;
		ma = ry;
	}
	break;
	case Face::pZ:
	{

		sc = rx;
		tc = -ry;
		ma = rz;
	}
	break;
	case Face::nZ:
	{

		sc = -rx;
		tc = -ry;
		ma = rz;
	}
	break;
	}

	float u = (sc / std::abs(ma) + 1.0f) / 2.0f;
	float v = (tc / std::abs(ma) + 1.0f) / 2.0f;

	return { u, v, majorAxis };
}
void CubeImage::fixupEdges(Face srcFace, size_t srcX, size_t srcY, size_t srcWalk, size_t srcPerpWalk,
                           Face neighbourFace, size_t neighbourX, size_t neighbourY, size_t neighbourWalk,
                           size_t neighbourPerpWalk)
{
	Image2D* srcImage = faceImages[srcFace];
	Image2D* neighbourImage = faceImages[neighbourFace];

	// assume both images have the same dimensions
	size_t dim = srcImage->getWidth();

	//maximum width of fixup region is one half of the cube face size
	uint32_t fixupDist = dim / 2;

	// note that this loop does not process the corner texels, since they have already been
	//  averaged across faces across earlier
	for (size_t j = 1; j < dim - 1; ++j)
	{
		// for each set of taps along edge, average them
		// and rewrite the results into the edges
		for (uint8_t k = 0; k < channels; ++k)
		{
			uint32_t edgeTap = srcImage->getTexel(srcX, srcY, k);
			uint32_t neighborEdgeTap = neighbourImage->getTexel(neighbourX, neighbourY, k);

			//compute average of tap intensity values
			uint32_t avgTap = 0.5f * (edgeTap + neighborEdgeTap);

			//propagate average of taps to edge taps
			srcImage->writeTexel(avgTap, srcX, srcY, k);
			neighbourImage->writeTexel(avgTap, neighbourX, neighbourY, k);

			uint32_t edgeTapDev = edgeTap - avgTap;
			uint32_t neighborEdgeTapDev = neighborEdgeTap - avgTap;

			// iterate over taps in direction perpendicular to edge, and
			//  adjust intensity values gradualy to obscure change in intensity values of
			//  edge averaging.
			for (size_t fixup = 1.0f; fixup < fixupDist; fixup++)
			{
				// fractional amount to apply change in tap intensity along edge to taps
				//  in a perpendicular direction to edge
				float fixupWeight = static_cast<float>(fixupDist - fixup) / static_cast<float>(fixupDist);

				// perform weighted average of edge tap value and current tap
				// fade off weight linearly as a function of distance from edge
                float srcPixel = srcImage->getTexel(j, fixup * srcPerpWalk, k) - avgTap;
                float nPixel = neighbourImage->getTexel(j, fixup * neighbourPerpWalk, k) - avgTap;
                
				// vary intensity of taps within fixup region toward edge values to hide changes made to edge taps
                srcPixel -= fixupWeight * srcPixel;
                nPixel -= fixupWeight * nPixel;
                
                srcImage->writeTexel(srcPixel, j, fixup * srcPerpWalk, k);
                neighbourImage->writeTexel(nPixel, j, fixup * srcPerpWalk, k);
			}
		}
	}
}

void CubeImage::seamlessEdges(uint32_t dim, uint8_t channels)
{

	// For images of size 1 x 1 we just average the face colours
	if (dim == 1)
	{
		for (size_t k = 0; k < channels; ++k)
		{
			float accum = 0.0f;

			// collect face colours
			for (size_t face = 0; face < 6; ++face)
			{
                accum += *(faceImages[face]->getData() + k);
			}

			// and average the accumulated colour over the six faces
			accum /= 6.0f;

			for (size_t face = 0; face < 6; ++face)
			{
				*(faceImages[face]->getData() + k) = accum;
			}
		}

		return;
	}

	// start by averaging the corner values
	for (uint8_t face = 0; face < 6; ++face)
	{
		// get the image for each face
		Image2D* image = faceImages[face];

		// average for each channel
		for (uint8_t k = 0; k < channels; k++)
		{
			float accum = 0.0f;
			for (uint8_t i = 0; i < 3; i++)
			{
				// top left
				image->writeTexel((image->getTexel(0, 0, k) + image->getTexel(-1, 0, k) + image->getTexel(0, -1, k)) / 3,
				                -1, -1, k);
				// top right
				image->writeTexel(
				    (image->getTexel(dim - 1, 0, k) + image->getTexel(dim - 1, -1, k) + image->getTexel(dim, 0, k)) / 3,
				    dim, 1, k);
				// bottom left
				image->writeTexel(
				    (image->getTexel(-1, dim, k) + image->getTexel(0, dim - 1, k) + image->getTexel(0, dim, k)) / 3, -1,
				    dim, k);
				// bottom right
				image->writeTexel((image->getTexel(dim - 1, dim - 1, k) + image->getTexel(dim, dim - 1, k) +
				                 image->getTexel(dim, dim - 1, k)) /
				                    3,
				                dim, dim, k);
			}
		}
	}

    // TODO - this need completing
	// now fix up the cube edges
	size_t walkRowLeft = channels * dim;
	size_t walkRowRight = -(channels * dim);
	size_t walkPixelLeft = channels;
	size_t walkPixelRight = -channels;

	// top
	fixupEdges(Face::nX, 0, 0, walkRowLeft, walkPixelLeft, Face::pY, -1, 0, walkPixelLeft, walkRowLeft);    // left
	fixupEdges(Face::nZ, dim - 1, 0, walkPixelRight, walkRowLeft, Face::pY, 0, -1, walkPixelLeft,
	           walkRowLeft);    // top
	fixupEdges(Face::pX, dim - 1, 0, walkPixelRight, walkRowLeft, Face::pY, dim, 0, walkRowLeft,
	           walkPixelLeft);                                                                               // right
	fixupEdges(Face::pZ, 0, 0, walkPixelLeft, walkRowLeft, Face::pY, 0, dim, walkPixelLeft, walkRowLeft);    // bottom
}

Image2DF32::Colour3 CubeImage::fetchBilinear(float x, float y, Image2DF32* data)
{
	const size_t x0 = static_cast<size_t>(x);
	const size_t y0 = static_cast<size_t>(y);
	const size_t x1 = x0 + 1;
	const size_t y1 = y0 + 1;

	const float u = x0;
	const float v = y0;

	// get the four texels - x0-x1 and y0-y1
	Colour3 t0 = data->getTexel3(x0, y0);
	Colour3 t1 = data->getTexel3(x1, y0);
	Colour3 t2 = data->getTexel3(x0, y1);
	Colour3 t3 = data->getTexel3(x1, y1);

	return (1 - u) * (1 - v) * t0 + (u * 1 - v) * t1 + (1 - u * v) * t2 + u * v * t3;
}

Image2DF32::Colour3 CubeImage::fetchBilinear(OEMaths::vec3f& pos, CubeImage* cube)
{
	FaceTexCoords texCoords = calculateTexCoords(pos);

    Image2DF32* image = cube->faceImages[texCoords.face];

	float x =
	    std::min(texCoords.u * static_cast<float>(image->getDimensions()), static_cast<float>(image->getDimensions()));
	float y =
	    std::min(texCoords.v * static_cast<float>(image->getDimensions()), static_cast<float>(image->getDimensions()));

	return fetchBilinear(x, y, image);
}

Image2DF32::Colour3 CubeImage::fetchTrilinear(const OEMaths::vec3f& reflect, CubeImage* mip1, float lerp)
{
	// calculate the face and tex-coords from the reflected normal
	FaceTexCoords texCoords = calculateTexCoords(reflect);

	// get the face image for both mip levels
	Image2DF32* imageMip1 = faceImages[texCoords.face];          // level 0
	Image2DF32* imageMip2 = mip1->faceImages[texCoords.face];    // level 1

	float x0 = std::min(texCoords.u * static_cast<float>(imageMip1->getDimensions()),
	                    static_cast<float>(imageMip1->getDimensions()));
	float x1 = std::min(texCoords.u * static_cast<float>(imageMip2->getDimensions()),
	                    static_cast<float>(imageMip2->getDimensions()));
	float y0 = std::min(texCoords.v * static_cast<float>(imageMip1->getDimensions()),
	                    static_cast<float>(imageMip1->getDimensions()));
	float y1 = std::min(texCoords.v * static_cast<float>(imageMip2->getDimensions()),
	                    static_cast<float>(imageMip2->getDimensions()));

	// bilinear filter at level 0
	Colour3 t0 = fetchBilinear(x0, y0, imageMip1);

	// binlinear filter at level 1
	Colour3 t1 = fetchBilinear(x1, y1, imageMip2);

	return t0 * (1 - lerp) + t1 * lerp;
}

}    // namespace OmegaEngine
