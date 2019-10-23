#pragma once

#include "OEMaths/OEMaths.h"

#include "ImageUtils/Image2D.h"

#include <cstdint>
#include <vector>

namespace OmegaEngine
{

namespace Ibl
{

class BdrfImage
{
public:
    
    BdrfImage() = default;
    
    BdrfImage(const uint16_t samples, const size_t dim) :
        sampleCount(samples)
        dimensions(dim)
    {}
    
	// BDRF integration
	void integrate();

private:
    
    size_t dimensions = 512;
    uint16_t sampleCount = 1024;
    
    Image2D image;
    
};
	
class IrradianceImage
{
    IrradianceImage() = default;
    IrradianceImage(size_t dim, float phi, float theta, uint8_t mips) :
        dimensions(dim),
        dPhi(phi),
        dTheta(theta)
    {
        levels.resize(mips);
    }
    
    // irradiance map
	void prepare();

private:
    
    size_t dimensions = 1024;
    float dPhi = 0.035f;
    float dTheta = 0.025f;
    
    std::vector<CubeImage> levels;
};

class PreFilteredImage
{
public:
    
    // pre-filtered map
    void prepare(const uint16_t roughnessFactor, const uint16_t sampleCount);

private:
    
    std::vector<CubeImage> levels;
};

}
}    // namespace OmegaEngine
