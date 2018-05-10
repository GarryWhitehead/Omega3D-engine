#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (input_attachment_index = 0, binding = 2) uniform subpassInput positionSampler;
layout (input_attachment_index = 1, binding = 3) uniform subpassInput normalSampler;
layout (input_attachment_index = 2, binding = 4) uniform subpassInput albedoSampler;
layout (input_attachment_index = 3, binding = 5) uniform subpassInput aoSampler;
layout (input_attachment_index = 4, binding = 6) uniform subpassInput metallicSampler;
layout (input_attachment_index = 5, binding = 7) uniform subpassInput roughnessSampler;

layout (binding = 8) uniform sampler2DArray shadowSampler;
layout (binding = 9) uniform sampler2D BDRFlut;
layout (binding = 10) uniform samplerCube irradianceMap;
layout (binding = 11) uniform samplerCube prefilterMap;

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outFrag;

#define LIGHT_COUNT 15			// use uniform buffer
#define SHADOW_FACTOR 0.85

#define PI 3.1415926535897932384626433832795

struct Light
{
		vec4 pos;
		vec4 direction;
		vec4 colour;
		mat4 viewMatrix;
};

layout (binding = 1) uniform UboBuffer
{
	vec4 viewPos;
	Light lights[LIGHT_COUNT];
	vec3 cameraPos;
} ubo;

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

float GGX_Distribution(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	
	return (a2)/(PI * denom * denom); 
}

float GeometryShlickGGX(float NdotV, float NdotL, float roughness)
{
	float k = ((roughness + 1) * (roughness + 1)) / 8.0;
	float GV = NdotV / (NdotV * (1.0 - k) + k);
	float GL = NdotL / (NdotL * (1.0 - k) + k);
	return GL * GV;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 specularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness, vec3 albedo, vec3 lightCol)
{
	vec3 H = normalize (V + L);
	float NdotH = clamp(dot(N, H), 0.0, 1.0);
	float NdotV = clamp(dot(N, V), 0.0, 1.0);
	float NdotL = clamp(dot(N, L), 0.0, 1.0);

	vec3 colour = vec3(0.0);

	if (NdotL > 0.0) {
		
		float D = GGX_Distribution(NdotH, roughness); 
		float G = GeometryShlickGGX(NdotV, NdotL, roughness);
		vec3 F = FresnelSchlick(NdotV, F0);		
		
		vec3 specular = D * F * G / (4.0 * NdotL * NdotV + 0.001);		
		vec3 Kd = (vec3(1.0) - F) * (1.0 - metallic);			
		colour += (Kd * albedo / PI + specular) * NdotL;
	}

	return colour;
}

void main()
{
	vec3 finalColour;
	
	vec3 inPos = subpassLoad(positionSampler).rgb;
	vec3 N = subpassLoad(normalSampler).rgb;
	
	vec3 V = normalize(ubo.cameraPos - inPos);
	vec3 R = reflect(-V, N);
	
	// get colour information from G-buffer
	vec3 albedo = subpassLoad(albedoSampler).rgb;
	float ao = subpassLoad(aoSampler).r;
	float metallic = subpassLoad(metallicSampler).r;
	float roughness = subpassLoad(roughnessSampler).r;
	
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);
	
	// apply additional lighting contribution to specular 
	vec3 Lo = vec3(0.0);
	for(int c = 0; c < LIGHT_COUNT; c++) {
	
		vec3 L = normalize(ubo.lights[c].pos.xyz - inPos);
		Lo += specularContribution(L, V, N, F0, metallic, roughness, albedo, ubo.lights[c].colour.rgb);
	}
	
	float NdotV = max(dot(N, V), 0.0);
	vec3 F = FresnelRoughness(NdotV, F0, roughness);

	// diffuse
	vec3 irradiance = texture(irradianceMap, N).rgb;
	vec3 diffuse = irradiance * albedo;
	
	// specular
	const float MAX_REFLECTION_LOD = 9.0;
	vec3 prefilterCol = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 envBDRF = texture(BDRFlut, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 specular = prefilterCol * (F * envBDRF.x + envBDRF.y);
	
	// ambient
	vec3 Kd = 1.0 - F;
	Kd *= 1.0 - metallic;
	vec3 ambient = (Kd * diffuse + specular) * vec3(1.0) * ao;
	
	finalColour = ambient + Lo;
			
	// shadow calculations
	//vec3 fragPos = texelFetch(positionSampler, texUv, 0).rgb;
	
	//for(int i = 0; i < LIGHT_COUNT; i++) {
	
	//	vec4 shadowClip	= ubo.lights[i].viewMatrix * vec4(fragPos, 1.0);
	//	float shadowFactor = textureProj(shadowClip, i, vec2(0.0));
		
	//	finalColour *= shadowFactor;
	//}
	
	finalColour = finalColour / (finalColour + vec3(1.0));
	
	finalColour = pow(finalColour, vec3(1.0/2.2));
	
	outFrag = vec4(finalColour, 1.0);
}
		
		
		