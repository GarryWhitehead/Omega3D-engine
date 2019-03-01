#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform sampler2D baseColourMap;
layout (set = 0, binding = 1) uniform sampler2D normalMap;
layout (set = 0, binding = 2) uniform sampler2D mrrMap;
layout (set = 0, binding = 3) uniform sampler2D emissiveMap;
layout (set = 0, binding = 4) uniform sampler2D aoMap;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec3 inColour;
layout (location = 4) in vec3 inPos;

layout(push_constant) uniform pushConstants 
{
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	vec4 diffuseFactor;
	vec4 specularFactor;
	float metallicFactor;	
	float roughnessFactor;	
	float alphaMask;	
	float alphaMaskCutoff;
	bool haveBaseColourMap;
	bool haveNormalMap;
	bool haveEmissiveMap;
	bool haveMrMap;

} material;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec4 outColour;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec4 outPbr
layout (location = 4) out vec3 outEmissive;

#define EPSILON 0.00001

void main()
{
	// albedo
	vec4 baseColour;

	if (material.alphaMask == 1.0) {
		if (haveBaseColourMap) {
			baseColour = texture(baseColourMap, vUV) * material.baseColorFactor;
		}
		else {
			baseColour = material.baseColorFactor;
		}
		if (baseColour.a < material.alphaMaskCutoff) {
			discard;
		}

		baseColour *= inColour;		// vertex colour
	}

	// normal
	vec3 normal = normalize(inNormal);
	if (haveNormalMap) {

		vec3 tangent = normalize(inTangent.xyz);
        vec3 binormal = cross(normal, tangent) * inTangent.w;
        vec2 tangentSpace = texture(normalMap, inUV).xy * 2.0 - 1.0;
        normal = normalize(mat3(tangent, binormal, normal) * two_component_normal(tangent_space));
	}
	outNormal = normal;

	// diffuse - based on metallic and roughness
	float roughness = 0.0;
	float metallic = 0.0;
	if (material.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {

		roughness = material.roughnessFactor;
		metallic = material.metallicFactor;

		if (haveMrMap) {
			vec4 mrSample = texture(mrMap, inUv);
			roughness = mrSample.g * roughness;
			metallic = mrSample.b * metallic;
		} 
		else {
			roughness = clamp(roughness, 0.04, 1.0);
			metallic = clamp(metallic, 0.0, 1.0);
		}
	}

	if (material.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS) {
		// Values from specular glossiness workflow are converted to metallic roughness
		if (material.haveMrMap) {
			roughness = 1.0 - texture(mrMap, inUv).a;
		} else {
			roughness = 0.0;
		}

		vec4 diffuse = texture(colorMap, inUv);
		vec3 specular = texture(mrMap, inUv).rgb;

		float maxSpecular = max(max(specular.r, specular.g), specular.b);

		// Convert metallic value from specular glossiness inputs
		metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);

		vec3 baseColourDiffusePart = diffuse.rgb * ((1.0 - maxSpecular) / (1 - c_MinRoughness) / max(1 - metallic, EPSILON)) * material.diffuseFactor.rgb;
		vec3 baseColourSpecularPart = specular - (vec3(c_MinRoughness) * (1 - metallic) * (1 / max(metallic, EPSILON))) * material.specularFactor.rgb;
		baseColour = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), diffuse.a);

	}
	outColour = baseColour;
	outPbr = vec2(metallic, roughness);

	// ao
	float ambient = 1.0;
	if (haveAoMap) {
        ambient = texture(occlusionMap, inUv).x;
	}
	outColour.a = ambient;

	// emmisive
	vec3 emissve;
	if (haveEmissiveMap) {
        emissive = texture(uEmissiveMap, vUV).rgb;
		emissive *= material.emissiveFactor.rgb;
	}
	else { 
   		emissive = material.emissiveFactor.rgb;
	}
	outEmissive = emissive;
}