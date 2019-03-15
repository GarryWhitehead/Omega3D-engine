#version 450

#include "pbr.h"


// G buffers
layout (set = 1, binding = 0) uniform sampler2D positionSampler;
layout (set = 1, binding = 1) uniform sampler2D baseColourSampler;
layout (set = 1, binding = 2) uniform sampler2D normalSampler;
layout (set = 1, binding = 3) uniform sampler2D pbrSampler;
layout (set = 1, binding = 4) uniform sampler2D emissiveSampler;

// environment texture samplers
layout (set = 2, binding = 0) uniform sampler2D brdfLutSampler;
layout (set = 2, binding = 1) uniform samplerCube irradianceSampler;
layout (set = 2, binding = 2) uniform samplerCube prefilterSampler;

layout (location = 0) in vec2 inUv;

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

layout (set = 3, binding = 1) uniform UboBuffer
{
	vec4 cameraPos;
	Light lights[MAX_LIGHT_COUNT];
	float IBLAmbient;
	uint activeLightCount;
	bool useIBLContribution;
	
} ubo;

vec3 calculateIBL(vec3 N, float NdotV, float roughness, vec3 reflection, vec3 diffuseColour)
{
	const float MAX_REFLECTION_LOD = 4.0;
	vec2 brdf = texture(brdfLutSampler, vec2(NdotV, 1.0 - roughness)).rg;
	
	// specular contribution
	vec3 F = FresnelRoughness(NdotV, vec3(0.04), roughness);
	vec3 specularLight = textureLod(prefilterSampler, reflection, roughness * MAX_REFLECTION_LOD).rgb;
	vec3 specular = specularLight * (F * brdf.x + brdf.y);
	
	// diffuse contribution
	vec3 diffuseLight = texture(irradianceSampler, N).rgb;
	vec3 diffuse = diffuseLight * diffuseColour;
	
	diffuse *= ubo.IBLAmbient;
	specular *= ubo.IBLAmbient;
	
	return diffuse + specular;
}

void main()
{	
	vec3 inPos = texture(positionSampler, inUv).rgb;
	vec3 V = normalize(ubo.cameraPos.xyz - inPos);
	vec3 N = texture(normalSampler, inUv).rgb;
	vec3 R = -reflect(V, N);
	R.y *= -1.0;
	
	// get colour information from G-buffer
	vec3 baseColour = texture(baseColourSampler, inUv).rgb;
	float metallic = texture(pbrSampler, inUv).x;
	float roughness = texture(pbrSampler, inUv).y;
	float occlusion = texture(baseColourSampler, inUv).a;
	vec3 emissive = texture(emissiveSampler, inUv).rgb;
	
	vec3 F0 = vec3(0.04);
	vec3 specularColour = mix(F0, baseColour, metallic);
	
	float reflectance = max(max(specularColour.r, specularColour.g), specularColour.b);
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
	
	// apply additional lighting contribution to specular 
	vec3 colour = vec3(0.0);
	for(int c = 0; c < ubo.activeLightCount; c++) {  
		
		vec3 lightPos = ubo.lights[c].pos.xyz - inPos;
		float dist = length(lightPos);
		float attenuation = 1.0 / dist * dist;
		vec3 radiance = ubo.lights[c].colour.rgb * attenuation;
		
		vec3 L = normalize(ubo.lights[c].pos.xyz - inPos);
		colour += specularContribution(L, V, N, F0, metallic, roughness, baseColour, radiance);
	}
	
	// add IBL contribution if needed
	if (ubo.useIBLContribution) {
		float NdotV = max(dot(N, V), 0.0);
		colour += calculateIBL(N, NdotV, roughness, R, baseColour);
	}
	
	// occlusion
	colour = mix(colour, colour * occlusion, 1.0);
	
	// emissive 
	colour += emissive; 
		
	outFrag = vec4(colour, 1.0);
}
		
		
		