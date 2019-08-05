#version 450

#include "pbr.h"
#include "lights.h"
#include "shadow.h"

// G buffers
layout (set = 1, binding = 0) uniform sampler2D positionSampler;
layout (set = 1, binding = 1) uniform sampler2D baseColourSampler;
layout (set = 1, binding = 2) uniform sampler2D normalSampler;
layout (set = 1, binding = 3) uniform sampler2D pbrSampler;
layout (set = 1, binding = 4) uniform sampler2D emissiveSampler;

// shadow depth sampler
layout (set = 1, binding = 5) uniform sampler2D Depth_shadowSampler;

// environment texture samplers
layout (set = 2, binding = 0) uniform sampler2D brdfLutSampler;
layout (set = 2, binding = 1) uniform samplerCube irradianceSampler;
layout (set = 2, binding = 2) uniform samplerCube prefilterSampler;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inCameraPos;

layout (location = 0) out vec4 outFrag;

#define MAX_LIGHT_COUNT 50	// make sure this matches the manager - could use a specilization constant

layout (set = 0, binding = 2) uniform LightUbo
{
	SpotLight spotLights[MAX_LIGHT_COUNT];
	PointLight pointLights[MAX_LIGHT_COUNT];
} light_ubo;

layout (push_constant) uniform pushConstants
{
	float IBLAmbient;
	bool useIBLContribution;
} push;

vec3 calculateIBL(vec3 N, float NdotV, float roughness, vec3 reflection, vec3 diffuseColour, vec3 specularColour)
{	
	vec3 brdf = (texture(brdfLutSampler, vec2(NdotV, 1.0 - roughness))).rgb;
	
	// specular contribution
		// this should be a pbr input!
	const float maxLod = 5.0;
	
	float lod = maxLod * roughness;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	
	vec3 a = textureLod(prefilterSampler, reflection, lodf).rgb;
	vec3 b = textureLod(prefilterSampler, reflection, lodc).rgb;
	vec3 specularLight = mix(a, b, lod - lodf);
	
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
	vec3 N = texture(normalSampler, inUv).rgb;
	vec3 R = normalize(-reflect(V, N));
	
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
		
	// spot lights
	for(int i = 0; i < 2; ++i) 
	{  
		SpotLight light = light_ubo.spotLights[i];
		
		vec3 lightPos = light.pos.xyz - inPos;
		vec3 L = normalize(lightPos);
		float intensity = light.colour.a;
	
		float attenuation = calculateDistance(lightPos, light.fallOut);
		attenuation *= calculateAngle(light.direction.xyz, L, light.scale, light.offset); 	
		
		colour += specularContribution(L, V, N, baseColour, metallic, alphaRoughness, attenuation, intensity, light.colour.rgb, specReflectance, specReflectance90);
	}
	
	// point lights
	for(int i = 0; i < 1; ++i) 
	{  
		PointLight light = light_ubo.pointLights[i];
		
		vec3 lightPos = light.pos.xyz - inPos;
		vec3 L = normalize(lightPos);
		float intensity = light.colour.a;
		
		float attenuation = calculateDistance(lightPos, light.fallOut);
		colour += specularContribution(L, V, N, baseColour, metallic, alphaRoughness, attenuation, intensity, light.colour.rgb, specReflectance, specReflectance90);
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
	
	// finally adjust the colour if in shadow for each light source
	for(int i = 0; i < 2; i++) 
	{
		SpotLight light = light_ubo.spotLights[i];
		
		vec4 shadowClip	= light.viewMatrix * vec4(inPos, 1.0);
		float shadowFactor = shadowPCF(shadowClip, Depth_shadowSampler);
			
		outFrag *= shadowFactor;
	}
	
	for(int i = 0; i < 1; i++) 
	{
		PointLight light = light_ubo.pointLights[i];
		
		vec4 shadowClip	= light.viewMatrix * vec4(inPos, 1.0);
		float shadowFactor = shadowPCF(shadowClip, Depth_shadowSampler);
			
		outFrag *= shadowFactor;
	}
}
		
		
		