#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 2) uniform sampler2DMS positionSampler;
layout (binding = 3) uniform sampler2DMS normalSampler;
layout (binding = 4) uniform sampler2DMS albedoSampler;
layout (binding = 5) uniform sampler2DArray shadowSampler;

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outFrag;

#define LIGHT_COUNT 3
#define SHADOW_FACTOR 0.25

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
} ubo;

layout (constant_id = 0) const int SAMPLE_COUNT = 8;

vec4 resolve(sampler2DMS texture, ivec2 uv)
{
	vec4 result = vec4(0.0);
	for(int c = 0; c < SAMPLE_COUNT; c++) {
	
		vec4 texel = texelFetch(texture, uv, c);
		result += texel;
	}
	result /= float(SAMPLE_COUNT);
	return result;
}


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

vec3 SampleLight(vec3 fragPos, vec3 normal, vec4 albedo)
{
	vec3 light = vec3(0.0);
	
	vec3 N = normalize(normal);
	
	for(int i = 0; i < LIGHT_COUNT; i++) {
	
		// calculate light direction
		vec3 lightDir = normalize(ubo.lights[i].pos.xyz - fragPos);

		vec3 viewDir = normalize(ubo.viewPos.xyz - fragPos);
		
		// diffuse lighting
		float diff = max(dot(N, lightDir), 0.0); 
		vec3 diffuse = vec3(diff);
		
		// specular
		vec3 R = reflect(-lightDir, N);
		float angle = max(0.0, dot(R, viewDir));
		vec3 specular = vec3(pow(angle, 16.0) * albedo.a * 2.5);
		
		light += vec3(diffuse + specular) * ubo.lights[i].colour.rgb * albedo.rgb;
	}
	
	return light;
}

void main()
{
	vec3 finalColour = vec3(0.0);
	
	// convert to non-normalised uv co-ords for use with texels
	ivec2 texDim = textureSize(positionSampler);
	ivec2 texUv = ivec2(texDim * inUv);
	
	// ambient
	vec4 resAlbedo = resolve(albedoSampler, texUv);
	
	// diffuse and specular for each sample
	for(int c = 0; c < SAMPLE_COUNT; c++) {
	
		vec3 texPos = texelFetch(positionSampler, texUv, c).rgb;
		vec3 normal = texelFetch(normalSampler, texUv, c).rgb;
		vec4 albedo = texelFetch(albedoSampler, texUv, c); 
		finalColour += SampleLight(texPos, normal, albedo);
	}
	
	finalColour = (resAlbedo.rgb * 0.5) + finalColour / float(SAMPLE_COUNT);
			
	// shadow calculations
	vec3 fragPos = texelFetch(positionSampler, texUv, 0).rgb;
	
	for(int i = 0; i < LIGHT_COUNT; i++) {
	
		vec4 shadowClip	= ubo.lights[i].viewMatrix * vec4(fragPos, 1.0);
		float shadowFactor = textureProj(shadowClip, i, vec2(0.0));
		
		finalColour *= shadowFactor;
	}
	
	outFrag = vec4(finalColour, 1.0);
}
		
		
		