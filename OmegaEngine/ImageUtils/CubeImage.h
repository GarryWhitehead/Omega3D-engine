#pragma once

#include "Image2D.h"

#include <cstdint>
#include <array>

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
    
    void seamlessEdges(uint32_t dim, uint8_t channels)
    {
       int32 face;
       int32 edge;
       int32 neighborFace;
       int32 neighborEdge;
    
       CPCubeMapNeighbor neighborInfo;
    
       CP_ITYPE* edgeStartPtr;
       CP_ITYPE* neighborEdgeStartPtr;
    
       int32 edgeWalk;
       int32 neighborEdgeWalk;
    
       //pointer walk to walk one texel away from edge in perpendicular direction
       int32 edgePerpWalk;
       int32 neighborEdgePerpWalk;
    
       // note that if functionality to filter across the three texels for each corner, then
       CP_ITYPE *cornerPtr[8][3];      //indexed by corner and face idx
       CP_ITYPE *faceCornerPtrs[4];    //corner pointers for face
       int32 cornerNumPtrs[8];         //indexed by corner and face idx
       int32 iFace;                    //iterator for faces
       int32 corner;
        
       
        enum class EdgeType : uint8_t
        {
            Left,
            Right,
            Top,
            Bottom
        };
        
        std::array<int32_t, 8> cornerNumPtrs;
        
       // For images of size 1 x 1 we just average the face colours
       if (dim == 1)
       {
          for (size_t k = 0; k < nChannels; ++k)
          {
             float accum = 0.0f;
    
             // collect face colours
             for(size_t face = 0; face < 6; ++face)
             {
                accum += *(faceImages[iFace].getData() + k);
             }
    
             // and average the accumulated colour over the six faces
             accum /= 6.0f;
    
             for(size_t face = 0; face < 6; ++face)
             {
                *(faceImages[iFace].getData() + k = accum;
             }
          }
    
          return;
       }
    
    
       // generate corner texel pointers
       for(uint8_t face = 0; uint8_t face < 6; ++face)
       {
          //the 4 corner pointers for this face
          faceCornerPtrs[0] = faceImages[iFace].m_ImgData;
          faceCornerPtrs[1] = faceImages[iFace].m_ImgData + ( (dim - 1) * channels );
          faceCornerPtrs[2] = faceImages[iFace].m_ImgData + ( (dim) * (dim - 1) * channels );
          faceCornerPtrs[3] = faceImages[iFace].m_ImgData + ( (((dim) * (dim - 1)) + (dim - 1)) * channels );
    
          //iterate over face corners to collect cube corner pointers
          for(uint8_t i = 0; i < 4; ++i)
          {
             corner = sg_CubeCornerList[iFace][i];
             cornerPtr[corner][ cornerNumPtrs[corner] ] = faceCornerPtrs[i];
             cornerNumPtrs[corner]++;
          }
       }
    
    
       // start by averaging the corner values
       for(uint8_t corner = 0; corner < 8; corner++)
       {
          for(uint8_t k = 0; k < channels; k++)
          {
             float accum = 0.0f;
             for(uint8_t i = 0; i < 3; i++)
             {
                accum += *(cornerPtr[iCorner][i] + k);
             }
    
             // average the accumulatd colour
             accum *= (1.0f / 3.0f);
    
             for(uint8_t i = 0; i < 3; i++)
             {
                *(cornerPtr[iCorner][i] + k) = cornerTapAccum;
             }
          }
       }
        
        struct EdgeInfo
        {
            float* dataPtr = nullptr;
            size_t walk;
            size_t perpWalk;    // used for fixing up the edge
        };
                  
       //maximum width of fixup region is one half of the cube face size
       uint32_t fixupDist = std::min(fixupWidth, dim / 2);
                  
       //iterate over the twelve edges of the cube to average across edges
       for(uint8_t i = 0; i < 12; i++)
       {
          face = sg_CubeEdgeList[i][0];
          edge = sg_CubeEdgeList[i][1];
         
            
          EdgeInfo firstEdge {faceImages[face].getData()
                    
          neighbourEdge;
          
          neighborInfo = sg_CubeNgh[face][edge];
          neighborFace = neighborInfo.m_Face;
          neighborEdge = neighborInfo.m_Edge;
    
          edgeStartPtr = faceImages[face].getData();
          neighborEdgeStartPtr = faceImages[neighborFace].getData();
          edgeWalk = 0;
          neighborEdgeWalk = 0;
    
          //amount to pointer to sample taps away from cube face
          edgePerpWalk = 0;
          neighborEdgePerpWalk = 0;
                
            
                    
          // calculate pointers based on which edge we are processing
          switch(edge)
          {
              case EdgeType::Left:
                // no change to faceEdgeStartPtr
                edgeWalk = channels * size;
                edgePerpWalk = channels;
             break;
              case EdgeType::Right:
                edgeStartPtr += dim - 1 * channels;
                edgeWalk = channels * dim;
                edgePerpWalk = -nchannels;
             break;
              case EdgeType::Top:
                // no change to faceEdgeStartPtr
                edgeWalk = channels;
                edgePerpWalk = channels * dim;
             break;
              case EdgeType::Bottom:
                edgeStartPtr += dim * (dim - 1) * channels;
                edgeWalk = channels;
                edgePerpWalk = -(channels * dim);
             break;
          }
    
          // If the edge enums are the same, or the sum of the enums == 3,
          //  the neighbor edge walk needs to be flipped
          if( (edge == neighborEdge) || ((edge + neighborEdge) == 3) )
          {
             switch(neighborEdge)
             {
                case EdgeType::Left:  //start at lower left and walk up
                   neighborEdgeStartPtr += (dim - 1) * dim *  channels;
                   neighborEdgeWalk = -(channels * dim);
                   neighborEdgePerpWalk = channels;
                break;
                case EdgeType::Right: //start at lower right and walk up
                   neighborEdgeStartPtr += ((dim - 1) * dim + (dim - 1)) * channels;
                   neighborEdgeWalk = -(channels * dim);
                   neighborEdgePerpWalk = -channels;
                break;
                case EdgeType::Top:   //start at upper right and walk left
                   neighborEdgeStartPtr += (dim - 1) * channels;
                   neighborEdgeWalk = -channels;
                   neighborEdgePerpWalk = (channels * dim);
                break;
                case EdgeType::Bottom: //start at lower right and walk left
                   neighborEdgeStartPtr += ((dim - 1) * dim + (dim - 1)) * channels;
                   neighborEdgeWalk = -channels;
                   neighborEdgePerpWalk = -(channels * dim);
                break;
             }
          }
          else
          {
             switch(neighborEdge)
             {
                case EdgeType::Left: //start at upper left and walk down
                   neighborEdgeWalk = channels * dim;
                   neighborEdgePerpWalk = channels;
                break;
                case EdgeType::Right: //start at upper right and walk down
                   neighborEdgeStartPtr += (dim - 1) * channels;
                   neighborEdgeWalk = channels * dim;
                   neighborEdgePerpWalk = -channels;
                break;
                case EdgeType::Top:   //start at upper left and walk left
                   //no change to neighborEdgeStartPtr for this case since it points
                   // to the upper left corner already
                   neighborEdgeWalk = channels;
                   neighborEdgePerpWalk = (channels * dim);
                break;
                case EdgeType::Bottom: //start at lower left and walk left
                   neighborEdgeStartPtr += dim * (dim - 1) * channels;
                   neighborEdgeWalk = channels;
                   neighborEdgePerpWalk = -(channels * size);
                break;
             }
          }
    
    
          //Perform edge walk, to average across the 12 edges and smoothly propagate change to
          //nearby neighborhood
          //step ahead one texel on edge
          edgeStartPtr += edgeWalk;
          neighborEdgeStartPtr += neighborEdgeWalk;
    
          // note that this loop does not process the corner texels, since they have already been
          //  averaged across faces across earlier
          for(size_t j = 1; j < dim - 1; ++j)
          {
             // for each set of taps along edge, average them
             // and rewrite the results into the edges
             for(uint8_t k = 0; k < channels; ++k)
             {
                uint32_t edgeTap, neighborEdgeTap, avgTap;  //edge tap, neighborEdgeTap and the average of the two
                uint32_t edgeTapDev, neighborEdgeTapDev;
    
                edgeTap = *(edgeStartPtr + k);
                neighborEdgeTap = *(neighborEdgeStartPtr + k);
    
                //compute average of tap intensity values
                avgTap = 0.5f * (edgeTap + neighborEdgeTap);
    
                //propagate average of taps to edge taps
                *(edgeStartPtr + k) = avgTap;
                *(neighborEdgeStartPtr + k) = avgTap;
    
                edgeTapDev = edgeTap - avgTap;
                neighborEdgeTapDev = neighborEdgeTap - avgTap;
    
                //iterate over taps in direction perpendicular to edge, and
                //  adjust intensity values gradualy to obscure change in intensity values of
                //  edge averaging.
                for(size_t fixup = 1; fixup < fixupDist; fixup++)
                {
                   //fractional amount to apply change in tap intensity along edge to taps
                   //  in a perpendicular direction to edge
                   float fixupWeight = static_cast<float>(fixupDist - fixup) / static_cast<float>(fixupDist);
    
                     //perform weighted average of edge tap value and current tap
                     // fade off weight linearly as a function of distance from edge
                     edgeTapDev =
                        (*(edgeStartPtr + (iFixup * edgePerpWalk) + k)) - avgTap;
                     neighborEdgeTapDev =
                        (*(neighborEdgeStartPtr + (iFixup * neighborEdgePerpWalk) + k)) - avgTap;
    
                   // vary intensity of taps within fixup region toward edge values to hide changes made to edge taps
                   *(edgeStartPtr + (iFixup * edgePerpWalk) + k) -= (fixupWeight * edgeTapDev);
                   *(neighborEdgeStartPtr + (iFixup * neighborEdgePerpWalk) + k) -= (fixupWeight * neighborEdgeTapDev);
                }
    
             }
    
             edgeStartPtr += edgeWalk;
             neighborEdgeStartPtr += neighborEdgeWalk;
          }
       }
    }

	Colour3 fetchBilinear(size_t x, size_t y, const Face face);
    

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

	Image2D* faceImages[6];
}

}    // namespace OmegaEngine
