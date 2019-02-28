#version 450

#include "pbr.h"

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// inputs from subpass
layout (set = 0, binding = 0) uniform sampler positionSampler;
layout (set = 0, binding = 1) uniform sampler albedoSampler;
layout (set = 0, binding = 2) uniform sampler normalSampler;
layout (set = 0, binding = 3) uniform sampler pbrSampler;

// texture samplers
layout (set = 0, binding = 4) uniform sampler2D BDRFlut;
layout (set = 0, binding = 5) uniform samplerCube irradianceMap;
layout (set = 0, binding = 6) uniform samplerCube prefilterMap;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inPosW;

layout (location = 0) out vec4 outFrag;

#define MAX_LIGHT_COUNT 5			// use uniform buffer
#define SHADOW_FACTOR 0.25

struct Light
{
		mat4 viewMatrix;
		vec4 pos;
		vec4 direction;
		vec4 colour;
};

layout (binding = 1) uniform UboBuffer
{
	vec4 cameraPos;
	Light lights[MAX_LIGHT_COUNT];
	
} ubo;

layout (push_constant) uniform pushConstant
{
	uint activeLightCount;
} push;

float textureProj(vec4 P, float layer, vec2 offset)
{
	float shadow = 1.0;
	vec4 shadowCoord = P / P.w;
	shadowCoord.st = shadowCoord.st * 0.5 + 0.5;
	
	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) 
	{
		float dist = texture(shadowSampler, vec3(shadowCoord.st + offset, layer)).r;
		if (shadowCoord.w > 0.0 && dist < shadowCoord.z) 
		{
			shadow = SHADOW_FACTOR;
		}
	}
	return shadow;
}

vec3 perturbNormal(vec3 posW)
{
	vec3 tangentNormal = subpassLoad(bumpSampler).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(posW);
	vec3 q2 = dFdy(posW);
	vec2 st1 = dFdx(inUv);
	vec2 st2 = dFdy(inUv);

	vec3 N = normalize(subpassLoad(normalSampler).rgb);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main()
{	
	vec3 inPos = positionSampler.rgb;
	vec3 V = normalize(ubo.cameraPos.xyz - inPos);
	
	vec3 N = perturbNormal(inPos);
	vec3 R = reflect(-V, N);

	// get colour information from G-buffer
	vec3 albedo = albedoSampler.rgb;
	float metallic = pbrSampler.x;
	float roughness = pbrSampler.y;
	
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);
	
	// apply additional lighting contribution to specular 
	vec3 Lo = vec3(0.0);
	for(int c = 0; c < push.activeLightCount; c++) {  
		
		vec3 lightPos = ubo.lights[c].pos.xyz - inPos;
		float dist = length(lightPos);
		float atten = 1.0 / dist * dist;
		vec3 radiance = ubo.lights[c].colour.rgb * atten;
		
		vec3 L = normalize(ubo.lights[c].pos.xyz - inPos);
		Lo += specularContribution(L, V, N, F0, metallic, roughness, albedo, radiance);
	}
	
	float NdotV = max(dot(N, V), 0.0);
	vec3 F = FresnelRoughness(NdotV, F0, roughness);

	// diffuse
	vec3 irradiance = texture(irradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo;
	
	// specular
	const float MAX_REFLECTION_LOD = 4.0;
	vec3 prefilterCol = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 envBDRF = texture(BDRFlut, vec2(max(dot(N, V), 0.0), roughness)).rg;

	vec3 specular = prefilterCol * (F * envBDRF.x + envBDRF.y);
	
	// ambient
	vec3 Kd = 1.0 - F;
	Kd *= 1.0 - metallic;
	vec3 ambient = (Kd * diffuse + specular) * subpassLoad(aoSampler).rrr;

	vec3 finalColour = ambient + Lo;
			
	// shadow calculations
	
	for(int i = 0; i < push.activeLightCount; i++) {
	
		vec4 shadowClip	= ubo.lights[i].viewMatrix * vec4(inPos, 1.0);
		float shadowFactor = textureProj(shadowClip, i, vec2(0.0));
		
		finalColour *= shadowFactor;
	}
	
	outFrag = vec4(finalColour, 1.0);
}
		
		
		