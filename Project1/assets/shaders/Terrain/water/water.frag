#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define BLENDING_BEGIN 800
#define BLENDING_END 20000
#define PERLIN_DIM 64

layout (set = 0, binding = 2) uniform sampler2D gradientSampler;
layout (set = 0, binding = 3) uniform sampler2D noiseSampler;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inPos;
layout (location = 2) in vec3 inViewDir;

layout (location = 0) out vec4 outColour;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outAlbedo;
layout (location = 4) out vec4 outBump;
layout (location = 5) out float outAo;
layout (location = 6) out float outMetallic;
layout (location = 7) out float outRoughness;

layout (set = 0, binding = 4) uniform UBOBuffer
{
	vec4 perlinOctave;
	vec4 perlinGradient;
	float perlinMovement;
	float texelSize2x;
} ubo;

void main()
{	
	// colour is blended with noise at futher distances to reduce tiling artifacts
	float viewLen = length(inViewDir);
	float blendFactor = (BLENDING_END - viewLen) / (BLENDING_END - BLENDING_BEGIN);
	blendFactor = clamp(blendFactor * blendFactor * blendFactor, 0.0, 1.0);
	
	vec2 perlin_x = vec2(0.0);
	vec2 perlin_y = vec2(0.0);
	vec2 perlin_z = vec2(0.0);
	vec2 perlinUv = inUv * PERLIN_DIM + (0.5 / 512);
	
	if(blendFactor < 1) {
		perlin_x = perlinUv * ubo.perlinOctave.x * ubo.perlinMovement;
		perlin_y = perlinUv * ubo.perlinOctave.y * ubo.perlinMovement;
		perlin_z = perlinUv * ubo.perlinOctave.z * ubo.perlinMovement;
	}
	
	vec2 noise = (perlin_x * ubo.perlinGradient.x + perlin_y * ubo.perlinGradient.y + perlin_z * ubo.perlinGradient.z);
	
	// calculate normal which is dependent on noise 
	vec2 gradUv = (blendFactor > 0) ? inUv : vec2(0.0);
	vec2 N = texture(gradientSampler, gradUv).xy;	
	N = mix(noise, N, blendFactor);
	outNormal = vec4(N, ubo.texelSize2x, 0.0);			// a channel informs deferred not to perturb normal
		
	// output fragment information to the appropiate buffers - lighting will be calculated in the deferred pass	
	outAlbedo = vec4(0.07, 0.15, 0.2, 1.0);			// default water colour

	// position
	outPosition = vec4(inPos, 1.0);
	
	// bump map
	outBump = vec4(0.0, 0.0, -1.0, 1.0);

	// ao, metallic and roughness arent used for water
	outAo = 0.4;
	outMetallic.r = 0.2;
	outRoughness.r = 0.5;

	outColour = vec4(0.0);
}