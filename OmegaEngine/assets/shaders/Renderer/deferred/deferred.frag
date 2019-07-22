#version 450

#include "pbr.h"


// G buffers
layout (set = 1, binding = 0) uniform sampler2D positionSampler;
layout (set = 1, binding = 1) uniform sampler2D baseColourSampler;
layout (set = 1, binding = 2) uniform sampler2D normalSampler;
layout (set = 1, binding = 3) uniform sampler2D pbrSampler;
layout (set = 1, binding = 4) uniform sampler2D emissiveSampler;

// shadow depth sampler
layout (set = 1, binding = 5) uniform sampler2D shadowSampler;

// environment texture samplers
layout (set = 2, binding = 0) uniform sampler2D brdfLutSampler;
layout (set = 2, binding = 1) uniform samplerCube irradianceSampler;
layout (set = 2, binding = 2) uniform samplerCube prefilterSampler;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inCameraPos;

layout (location = 0) out vec4 outFrag;

#define MAX_LIGHT_COUNT 100	// make sure this matches the manager - could use a specilization constant

// light types - must reflect main code enums
#define SPOTLIGHT 0
#define CONE 1

struct Light
{
		vec3 pos;
		float pad0;
		vec3 target;
		float pad1; 
		vec3 colour;
		float fov;
		float radius;
		float innerCone;
		float outerCone;
		uint type;
};

layout (set = 0, binding = 2) uniform LightUbo
{
	Light lights[MAX_LIGHT_COUNT];
	uint activeLightCount;
} light_ubo;

layout (push_constant) uniform pushConstants
{
	float IBLAmbient;
	bool useIBLContribution;
} push;

vec3 calculateIBL(vec3 N, float NdotV, float roughness, vec3 reflection, vec3 diffuseColour, vec3 specularColour)
{
	// this should be a pbr input!
	const float MAX_REFLECTION_LOD = 5.0;
	vec3 brdf = texture(brdfLutSampler, vec2(NdotV, 1.0 - roughness)).rgb;
	
	// specular contribution
	vec3 specularLight = textureLod(prefilterSampler, reflection, roughness * MAX_REFLECTION_LOD).rgb;
	vec3 specular = specularLight * (specularColour * brdf.x + brdf.y);
	
	// diffuse contribution
	vec3 diffuseLight = texture(irradianceSampler, N).rgb;
	vec3 diffuse = diffuseLight * diffuseColour;
	
	diffuse *= push.IBLAmbient;
	specular *= push.IBLAmbient;
	
	return diffuse + specular;
}

void main()
{	
	vec3 inPos = texture(positionSampler, inUv).rgb;
	vec3 V = normalize(inCameraPos.xyz - inPos);
	vec3 N = normalize(texture(normalSampler, inUv)).rgb;
	vec3 R = -normalize(reflect(V, N));
	
	// get colour information from G-buffer
	vec3 baseColour = texture(baseColourSampler, inUv).rgb;
	float metallic = texture(pbrSampler, inUv).x;
	float roughness = texture(pbrSampler, inUv).y;
	float occlusion = texture(baseColourSampler, inUv).a;
	vec3 emissive = texture(emissiveSampler, inUv).rgb;
	
	vec3 F0 = vec3(0.04);
	vec3 specularColour = mix(F0, baseColour, metallic);
	
	float reflectance = max(max(specularColour.r, specularColour.g), specularColour.b);
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);  // 25.0-50.0 is used
	vec3 specReflectance = specularColour.rgb;
	vec3 specReflectance90 = vec3(1.0, 1.0, 1.0) * reflectance90;
	
	float alphaRoughness = roughness * roughness;
	
	// apply additional lighting contribution to specular 
	vec3 colour = vec3(0.0);
	for(int c = 0; c < light_ubo.activeLightCount; c++) 
	{  
		vec3 lightPos = light_ubo.lights[c].pos.xyz - inPos;
		float dist = length(lightPos);
		vec3 L = normalize(lightPos);

		vec3 radiance;
		if (light_ubo.lights[c].type == SPOTLIGHT) 
		{
			float attenuation = light_ubo.lights[c].radius / (dist * dist);
			radiance = light_ubo.lights[c].colour.rgb * attenuation;
		}
		else if (light_ubo.lights[c].type == CONE) 
		{
			float innerCosAngle = cos(light_ubo.lights[c].innerCone);
			float outerCosAngle = cos(light_ubo.lights[c].outerCone);
			float dir = dot(L, lightPos);
			float spot = smoothstep(innerCosAngle, outerCosAngle, dir);
			float attenuation = smoothstep(light_ubo.lights[c].radius, 0.0f, dist);
			radiance = light_ubo.lights[c].colour.rgb * spot * attenuation;
		}
		
		colour += specularContribution(L, V, N, metallic, alphaRoughness, baseColour, radiance, specReflectance, specReflectance90);
	}
	
	// add IBL contribution if needed
	if (push.useIBLContribution) 
	{
		float NdotV = max(dot(N, V), 0.0);
		colour += calculateIBL(N, NdotV, roughness, R, baseColour, specularColour);
	}
	
	// occlusion
	colour = mix(colour, colour * occlusion, 1.0);
	
	// emissive 
	colour += emissive; 
		
	outFrag = vec4(colour, 1.0);
}
		
		
		