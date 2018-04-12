#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 2) uniform sampler2D positionSampler;
layout (binding = 3) uniform sampler2D normalSampler;
layout (binding = 4) uniform sampler2D albedoSampler;
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

void main()
{
	// extract values for position.etc from the G-buffer 
	vec3 fragPos = texture(positionSampler, inUv).rgb;
	vec3 normal = texture(normalSampler, inUv).rgb;
	vec4 albedo = texture(albedoSampler, inUv); 	
	vec3 light = albedo.rgb * 0.3;	// hard-coded ambient light - chnage to push-constant or similiar
	
	vec3 N = normalize(normal);
	
	for(int i = 0; i < LIGHT_COUNT; i++) {
	
		// calculate light direction
		vec3 lightDir = normalize(ubo.lights[i].pos.xyz - fragPos);

		vec3 viewDir = normalize(ubo.viewPos.xyz - fragPos);
		
		// diffuse lighting
		vec3 diffuse = max(dot(N, lightDir), 0.0) * albedo.rgb * ubo.lights[i].colour.rgb;
		
		// specular
		vec3 R = reflect(-lightDir, N);
		float angle = max(0.0, dot(R, viewDir));
		vec3 specular = vec3(pow(angle, 16.0) * albedo.a * 2.5);
		
		light += vec3(diffuse + specular);
	}
	
	
	for(int i = 0; i < LIGHT_COUNT; ++i) {
		vec4 shadowClip	= ubo.lights[i].viewMatrix * vec4(fragPos, 1.0);

		float shadowFactor;
		
		shadowFactor = textureProj(shadowClip, i, vec2(0.0));
		
		light *= shadowFactor;
	}
		
	
	
	outFrag.rgb = light;
}
		
		
		