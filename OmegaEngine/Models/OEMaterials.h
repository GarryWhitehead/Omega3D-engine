#pragma once
#pragma once
#include "OEMaths/OEMaths.h"

namespace OmegaEngine
{
namespace OEMaterials
{

namespace Specular
{
// stock materials - specular values
const OEMaths::vec3f Gold{ 1.0f, 0.765557f, 0.336057f };
const OEMaths::vec3f Copper{ 0.955008f, 0.637427f, 0.538163f };
const OEMaths::vec3f Chromium{ 0.549585f, 0.556114f, 0.554256f };
const OEMaths::vec3f Nickel{ 0.659777f, 0.608679f, 0.525649f };
const OEMaths::vec3f Titanium{ 0.541931f, 0.496791f, 0.449419f };
const OEMaths::vec3f Cobalt{ 0.541931f, 0.496791f, 0.449419f };
const OEMaths::vec3f Platinum{ 0.672411f, 0.637331f, 0.585456f };
} // namespace Specular

namespace Diffuse
{
// diffuse examples - in sRGB
constexpr float Charcoal = 0.04f;
constexpr float FreshAsphalt = 0.04f;
constexpr float WornAsphalt = 0.12f;
constexpr float Soil = 0.17f;
constexpr float GreenGrass = 0.25f;
constexpr float DesertSand = 0.4f;
constexpr float NewConcrete = 0.55f;
constexpr float OceanIce = 0.6f;
constexpr float FreshSnow = 0.9f;
} // namespace Diffuse

} // namespace OEMaterials

} // namespace OmegaEngine