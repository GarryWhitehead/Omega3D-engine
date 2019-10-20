#include "IblImage.h"

#include <algorithm>

namespace OmegaEngine
{

// ========= BDRF ===================
OEMaths::vec2f IblImage::hammersley(uint64_t i, uint64_t N)
{
	uint64_t bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rad = float(bits) * 2.3283064365386963e-10;    // 0x100000000
	return OEMaths::vec2f(float(i) / float(N), rad);
}

OEMaths::vec3f IblImage::GGX_ImportanceSample(OEMaths::vec2f Xi, OEMaths::vec3f N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * M_PI * Xi.getX();
	float cosTheta = sqrt((1.0f - Xi.getY()) / (1.0f + (a * a - 1.0f) * Xi.getY()));
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

	// spherical to cartesian coordinates
	OEMaths::vec3f H = OEMaths::vec3f(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	// from tangent-space vector to world-space sample vector
	OEMaths::vec3f up = abs(N.getZ()) < 0.999 ? OEMaths::vec3f(0.0f, 0.0f, 1.0f) : OEMaths::vec3f(1.0f, 0.0f, 0.0f);
	OEMaths::vec3f tanX = up.cross(N).normalise();
	OEMaths::vec3f tanY = N.cross(tanX).normalise();
	return OEMaths::vec3f::normalize(tanX * H.getX() + tanY * H.getY() + N * H.getZ());
}

float IblImage::geometryShlickGGX(float NdotV, float NdotL, float roughness)
{
	float k = (roughness * roughness) / 2.0f;
	float GV = NdotV / (NdotV * (1.0f - k) + k);
	float GL = NdotL / (NdotL * (1.0f - k) + k);
	return GL * GV;
}

OEMaths::vec2f IblImage::integrateBRDF(float NdotV, float roughness, uint16_t sampleCount)
{
	float NoV = inUv.s;

	OEMaths::vec3f N = OEMaths::vec3f(0.0f, 0.0f, 1.0f);
	OEMaths::vec3f V = OEMaths::vec3f(std::sqrt(1.0f - NoV * NoV), 0.0f, NoV);

	OEMaths::vec2f lut;
	for (int c = 0; c < sampleCount; c++)
	{

		OEMaths::vec2f Xi = hammersley(c, sampleCount);
		OEMaths::vec3f H = GGX_ImportanceSample(Xi, N, roughness);

		OEMaths::vec3f L = (2.0f * V.dot(H) * H - V);
		L.normalise();

		float NdotL = std::max(N.dot(L), 0.0f);
		float NdotH = std::max(N.dot(H), 0.0f);
		float NdotV = std::max(N.dot(V), 0.0f);
		float HdotV = std::max(H.dot(V), 0.0f);

		if (NdotL > 0.0f)
		{

			// cook-torrance BDRF calculations
			float G = geometryShlickGGX(NdotV, NdotL, roughness);
			float Gvis = (G * HdotV) / (NdotH * NdotV);
			float Fc = std::pow(1.0 - HdotV, 5.0);
			lut += OEMaths::vec2f((1.0 - Fc) * Gvis, Fc * Gvis);
		}
	}
	return lut / sampleCount;
}

//========================== irradiance ==============================

void IblImage::prepareIrradianceMap(float dPhi, float dTheta)
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
			irrColour += texture(envSampler, sampleVector).rgb * std::cos(theta) * std::sin(theta);

			// spherical to cartesian
			//vec3 tanSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

			// to world space
			//vec3 wPos = tanSample.x * right + tanSample.y * up + tanSample.z * N;

			//irrColour += texture(envSampler, wPos).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}

	outCol = vec4(PI * irrColour / float(sampleCount), 1.0);
}

// ==================== Pre-filtered ===============================

float IblImage::GGX_Distribution(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;

	return (a2) / (M_PI * denom * denom);
}

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