#include "IblImage.h"

#include "ImageUtils/IblUtils.h"

#include "Threading/ThreadPool.h"

#include <algorithm>

namespace OmegaEngine
{
namespace Ibl
{
    
    
// ========= BDRF ===================

void BdrfImage::integrate()
{
    for (float y = 0.0f; y < static_cast<float>(dimensions); ++y)
    {
        for (float x = 0.0f; x < static_cast<float>(dimensions); ++x)
        {
            OEMaths::vec3f N = OEMaths::vec3f(0.0f, 0.0f, 1.0f);
            OEMaths::vec3f V = OEMaths::vec3f(std::sqrt(1.0f - x * x), 0.0f, x);

            OEMaths::vec2f lut;
            for (int c = 0; c < sampleCount; c++)
            {
                OEMaths::vec2f Xi = hammersley(c, sampleCount);
                OEMaths::vec3f H = GGX_ImportanceSample(Xi, N, y);

                OEMaths::vec3f L = (H * 2.0f * V.dot(H)) - V;
                L.normalise();

                float NdotL = std::max(N.dot(L), 0.0f);
                float NdotH = std::max(N.dot(H), 0.0f);
                float NdotV = std::max(N.dot(V), 0.0f);
                float HdotV = std::max(H.dot(V), 0.0f);

                if (NdotL > 0.0f)
                {
                    // cook-torrance BDRF calculations
                    float G = geometryShlickGGX(NdotV, NdotL, y);
                    float Gvis = (G * HdotV) / (NdotH * NdotV);
                    float Fc = std::pow(1.0 - HdotV, 5.0);
                    lut += OEMaths::vec2f((1.0 - Fc) * Gvis, Fc * Gvis);
                }
                
                OEMaths::vec2f p = lut / (float)sampleCount;
                image.setPixel({p.getX(), p.getY()}, static_cast<uint32_t>(x), static_cast<uint32_t>(y));
            }
        }
    }
}

//========================== irradiance ==============================

void IrradianceImage::prepare()
{
    // the thread worker lamda
    auto worker = [&](const size_t workSize, Image2D* image, const uint32_t dim, const )
    {
        for (size_t y = 0; y < dim; ++y)
        {
            for (size_t x = 0; x < dim; ++x)
            {
                OEMaths::vec3f N = inPos.normalise();
                OEMaths::vec3f up = OEMaths::vec3f(0.0f, 1.0f, 0.0f);
                OEMaths::vec3f right = up.cross(N).normalise();
                up = N.cross(right);

                OEMaths::vec3f irrColour;
                float sampleCount = 0.0f;

                float doublePI = M_PI * 2;
                float halfPI = M_PI * 0.5;

                //const float dPhi = 0.035f;
                //const float dTheta = 0.025f;

                for (float phi = 0.0f; phi < doublePI; phi += dPhi)
                {
                    for (float theta = 0.0f; theta < halfPI; theta += dTheta)
                    {
                        OEMaths::vec3f tempVec = right * std::cos(phi) + up * std::sin(phi);
                        OEMaths::vec3f sampleVector = N * std::cos(theta) + tempVec * std::sin(theta);
                        
                        Colour3 texel = envImage.fetchTrilinear(sampleVector);
                        texel *= std::cos(theta) * std::sin(theta);
                        irrColour += texel;

                        // spherical to cartesian
                        //vec3 tanSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

                        // to world space
                        //vec3 wPos = tanSample.x * right + tanSample.y * up + tanSample.z * N;

                        //irrColour += texture(envSampler, wPos).rgb * cos(theta) * sin(theta);
                        ++sampleCount;
                    }
                }
                finalColour = irrColour * PI / static_cast<float>(sampleCount);
                image.setTexel(finalColour, x, y);
            }
        }
        
    };
    
    // a thread for each face
    ThreadPool threadPool(CubeImage::Face::Size);
 
	for (size_t face = 0; face < 6; ++face)
    {
		auto fut = threadPool.submitTask();
    }
}

// ==================== Pre-filtered ===============================

void IblImage::preparePreFiltered(const uint16_t roughnessFactor, const uint16_t sampleCount)
{
	OEMaths::vec3f N = normalize(inPos);
	OEMaths::vec3f  V = N;

	float totalWeight = 0.0f;
	OEMaths::vec3f preFilterCol;

	for (uint16_t c = 0; c < sampleCount; ++c)
	{
		OEMaths::vec2f Xi = hammersley(c, sampleCount);
		OEMaths::vec3f H = GGX_ImportanceSample(Xi, N, roughnessFactor);

		OEMaths::vec3f L = V - H * 2.0f * V.dot(H);	// CHECK!

		float NdotL = std::clamp(N.dot(L), 0.0f, 1.0f);
		float NdotH = std::clamp(N.dot(H), 0.0f, 1.0f);
		float HdotV = std::clamp(H.dot(V), 0.0f, 1.0f);

		if (NdotL > 0.0f)
		{

			float D = GGX_Distribution(NdotH, roughnessFactor);
			float pdf = (D * NdotH / (4.0f * HdotV)) + 0.0001f;

			float resolution = float(textureSize(envSampler, 0).s);
			float saTex = 4.0f * M_PI / (6.0f * resolution * resolution);
			float saSample = 1.0f / (static_cast<float>(sampleCount) * pdf + 0.0001f);

			float mipLevel = roughnessFactor == 0.0 ? 0.0 : std::max(0.5f * std::log2(saSample / saTex) + 1.0f, 0.0f);

			preFilterCol += textureLod(envSampler, L, mipLevel).rgb * NdotL;
			totalWeight += NdotL;
		}
	}

	outCol = vec4(preFilterCol / totalWeight, 1.0);
}

}
}
