#pragma once

#include "Image2D.h"

#include <array>
#include <cstdint>

namespace OmegaEngine
{

/**
	* 
	*/
class CubeImage : public Image2D<float>
{
public:
	///
	///
	/// +----------+
	/// | +---->+x |
	/// |        | |
	/// | |      +y|
	/// |+z 2      |
	/// +----------+----------+----------+----------+
	/// | +---->+z | +---->+x | +---->-z | +---->-x |
	/// |        | |        | |        | |        | |
	/// | | -x | | +z | | +x | | -z |
	/// |-y 1 |-y 4 |-y 0 |-y 5 |
	/// +----------+----------+----------+----------+
	/// | +---->+x |
	/// | | |
	/// | | -y |
	/// |-z 3 |
	/// +----------+
	///

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

	struct FaceTexCoords
	{
		float u;
		float v;
		Face face;
	};

	static OEMaths::vec3f calculateNormal(float x, float y, const Face face, float dim);

	/**
	* @brief Calculates the face tex coords for a given absolute reflected vector 
	*/
	static FaceTexCoords calculateTexCoords(OEMaths::vec3f reflect);

	void seamlessEdges(uint32_t dim, uint8_t channels);
	void fixupEdges(Face srcFace, size_t srcX, size_t srcY, size_t srcWalk, size_t srcPerpWalk, Face neighbourFace,
	                size_t neighbourX, size_t neighbourY, size_t neighbourWalk, size_t neighbourPerpWalk);

    static OEMaths::colour3 fetchBilinear(float x, float y, Image2DF32* data);

    static OEMaths::colour3 fetchBilinear(OEMaths::vec3f& pos, CubeImage* image);

    OEMaths::colour3 fetchTrilinear(const OEMaths::vec3f& reflect, CubeImage* mip1, float lerp);


private:
	Image2D<float>* faceImages[6];
};

}    // namespace OmegaEngine
