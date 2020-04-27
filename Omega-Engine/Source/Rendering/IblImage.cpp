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

#include "IblImage.h"

#include "ImageUtils/IblUtils.h"

#include "Threading/ThreadPool.h"

#include <algorithm>

namespace OmegaEngine
{
namespace Ibl
{

void IblImage::prepareBdrf()
{
	// ========= BDRF ===================
	auto bdrfThread = [&](Image2DF32* image, uint32_t dim, const size_t row) {
		for (float x = 0.0f; x < static_cast<float>(bdrf->dimensions); ++x)
		{
			OEMaths::vec3f N = OEMaths::vec3f(0.0f, 0.0f, 1.0f);
			OEMaths::vec3f V = OEMaths::vec3f(std::sqrt(1.0f - x * x), 0.0f, x);

			OEMaths::vec2f lut;
			for (int c = 0; c < bdrf->sampleCount; c++)
			{
				OEMaths::vec2f Xi = hammersley(c, bdrf->sampleCount);
				OEMaths::vec3f H = importanceSampleGGX(Xi, N, static_cast<float>(row));

				OEMaths::vec3f L = OEMaths::normalise(2.0f * OEMaths::dot(H, V) * H - V);

				float NdotL = std::max(OEMaths::dot(N, L), 0.0f);
				float NdotH = std::max(OEMaths::dot(N, H), 0.0f);
				float NdotV = std::max(OEMaths::dot(N, V), 0.0f);
				float HdotV = std::max(OEMaths::dot(H, V), 0.0f);

				if (NdotL > 0.0f)
				{
					// cook-torrance BDRF calculations
					float G = geometryShlickGGX(NdotV, NdotL, static_cast<float>(row));
					float Gvis = (G * HdotV) / (NdotH * NdotV);
					float Fc = std::pow(1.0 - HdotV, 5.0);
					lut += OEMaths::vec2f((1.0f - Fc) * Gvis, Fc * Gvis);
				}

				OEMaths::vec2f p = lut / (float)bdrf->sampleCount;
                Image2DF32::Colour2 finalColour { p.x, p.y };
                image->writeTexel2D(static_cast<uint32_t>(x), row, finalColour);
			}
		}
	};
}

void IblImage::prepare()
{
	//========================== irradiance ==============================

    auto irradianceThread = [&](Image2DF32* image, uint32_t dim, const size_t row, CubeImage::Face face) {
		for (size_t x = 0; x < dim; ++x)
		{
			OEMaths::vec3f N = CubeImage::calculateNormal(static_cast<float>(x), static_cast<float>(row), face,
			                                              static_cast<float>(dim));
			OEMaths::vec3f up = OEMaths::vec3f(0.0f, 1.0f, 0.0f);
			OEMaths::vec3f right = OEMaths::normalise(OEMaths::vec3f::cross(up, N));
			up = OEMaths::vec3f::cross(N, right);

			OEMaths::vec3f irrColour = { 0.0f };
			float sampleCount = 0.0f;

			for (float phi = 0.0f; phi < M_DBL_PI; phi += irradiance->dPhi)
			{
				for (float theta = 0.0f; theta < M_HALF_PI; theta += irradiance->dTheta)
				{
					// spherical to cartesian
					OEMaths::vec3f tanSample = OEMaths::vec3f(std::sin(theta) * std::cos(phi),
					                                          std::sin(theta) * std::sin(phi), std::cos(theta));

					// to world space
					OEMaths::vec3f wPos = tanSample.x * right + tanSample.y * up + tanSample.z * N;

					Image2DF32::Colour3 tex = CubeImage::fetchBilinear(wPos, envImage);
					irrColour += tex * cos(theta) * sin(theta);
					++sampleCount;
				}
			}

			Image2DF32::Colour3 finalColour = irrColour * M_PI / static_cast<float>(sampleCount);
			image->writeTexel3D(x, row, finalColour);
		}
	};


	// ==================== Pre-filtered ===============================

	auto preFilterThread = [&](Image2DF32* image, std::vector<CubeImage*>& cubeMips, const uint32_t dim, const uint32_t row, CubeImage::Face face, float roughness) {
		for (size_t x = 0; x < dim; ++x)
		{
			float maxLevel = static_cast<float>(prefilter->levels.size());
			OEMaths::vec3f N = CubeImage::calculateNormal(static_cast<float>(x), static_cast<float>(row), face, static_cast<float>(dim));
			OEMaths::vec3f V = N;

			float totalWeight = 0.0f;
			OEMaths::vec3f preFilterCol;

			for (uint16_t c = 0; c < prefilter->sampleCount; ++c)
			{
				OEMaths::vec2f Xi = hammersley(c, prefilter->sampleCount);
				OEMaths::vec3f H = importanceSampleGGX(Xi, N, roughness);

				OEMaths::vec3f L = OEMaths::normalise(2.0f * OEMaths::dot(H, V) * H - V);

				float NdotL = std::max(OEMaths::dot(N, L), 0.0f);
				float NdotH = std::max(OEMaths::dot(N, H), 0.0f);
				float HdotV = std::max(OEMaths::dot(H, V), 0.0f);

				if (NdotL > 0.0f)
				{

					float D = distributionGGX(NdotH, roughness);
					float pdf = (D * NdotH / (4.0f * HdotV)) + 0.0001f;

					float resolution = static_cast<float>(dim);
					float saTex = 4.0f * M_PI / (6.0f * resolution * resolution);
					float saSample = 1.0f / (static_cast<float>(prefilter->sampleCount) * pdf + 0.0001f);

					float mipLevel0 =
					    roughness == 0.0f ? 0.0f : std::max(0.5f * std::log2(saSample / saTex) + 1.0f, 0.0f);
					mipLevel0 = std::min(mipLevel0, maxLevel);
					float mipLevel1 = std::min(mipLevel0 + 1, maxLevel);

					CubeImage* cubeImageL0 = cubeMips[static_cast<uint64_t>(mipLevel0)];
					CubeImage* cubeImageL1 = cubeMips[static_cast<uint64_t>(mipLevel1)];

					preFilterCol += cubeImageL0->fetchTrilinear(L, cubeImageL1, mipLevel0) * NdotL;
					totalWeight += NdotL;
				}

				Image2DF32::Colour3 avgCol{ preFilterCol / totalWeight };
				image->writeTexel3D(x, row, avgCol);
			}
		}
	};

	// pre-filter
	for (uint32_t face = 0; face < 6; ++face)
	{
        Image2DF32* image = prefilter->levels[0]->getFace(static_cast<CubeImage::Face>(face));
		uint32_t dim = image->getDimensions();

		auto filterSplitTask = [&preFilterThread, &image, &dim](size_t curr_y, size_t chunkSize) {
			for (size_t y = curr_y; y < curr_y + chunkSize; ++y)
			{
				//preFilterThread(image, dim, y);
			}
		};

		ThreadTaskSplitter split{ 0, dim, filterSplitTask };
		split.run();
	}
}
}    // namespace Ibl
}    // namespace OmegaEngine
