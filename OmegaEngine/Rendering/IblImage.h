#pragma once

#include "OEMaths/OEMaths.h"

#include "ImageUtils/CubeImage.h"
#include "ImageUtils/Image2D.h"

#include <cstdint>
#include <vector>

namespace OmegaEngine
{

namespace Ibl
{

struct IblConfig
{
	size_t bdrfDim = 1024;
	uint16_t bdrfSampleCount = 1024;

	size_t irradianceDim = 1024;
	float dPhi = 0.035f;
	float dTheta = 0.025f;

	size_t preFilterDim = 512;
	
};

class IblImage
{
public:
	IblImage(IblConfig& config);

	// not copyable
	IblImage(const IblImage&) = delete;
	IblImage& operator=(IblImage&) = delete;

	IblImage() = default;

	struct IrradianceInfo
	{
		IrradianceInfo(size_t dim, float dPhi, float dTheta);

		IrradianceInfo() = default;

		// not copyable
		IrradianceInfo(IrradianceInfo&) = delete;
		IrradianceInfo& operator=(IrradianceInfo&) = delete;

		size_t dimensions = 1024;
		float dPhi = 0.035f;
		float dTheta = 0.025f;

		std::vector<CubeImage*> levels;
	};

	struct PreFilterInfo
	{
		PreFilterInfo(uint16_t samples);

		PreFilterInfo() = default;

		// not copyable
		PreFilterInfo(PreFilterInfo&) = delete;
		PreFilterInfo& operator=(PreFilterInfo&) = delete;

		uint16_t sampleCount = 1024;

		std::vector<CubeImage*> levels;
	};

	struct BdrfInfo
	{
		BdrfInfo(size_t dim, uint16_t samples);

		BdrfInfo() = default;

		// not copyable
		BdrfInfo(BdrfInfo&) = delete;
		BdrfInfo& operator=(BdrfInfo&) = delete;

		size_t dimensions = 512;
		uint16_t sampleCount = 1024;

		Image2DF32* image = nullptr;
	};

	void prepare();
	void prepareBdrf();

private:
	CubeImage* envImage = nullptr;

	PreFilterInfo* prefilter = nullptr;
	IrradianceInfo* irradiance = nullptr;
	BdrfInfo* bdrf = nullptr;
};

}    // namespace Ibl
}    // namespace OmegaEngine
