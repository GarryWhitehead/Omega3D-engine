/* Copyright (c) 2018-2020 Garry Whitehead
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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

    Image2D<float>* getFace(const Face face)
    {
        return faceImages[face];
    }
    
private:
	Image2D<float>* faceImages[6];
};

}    // namespace OmegaEngine
