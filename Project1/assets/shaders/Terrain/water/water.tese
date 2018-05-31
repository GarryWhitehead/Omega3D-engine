#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define BLENDING_BEGIN 800
#define BLENDING_END 20000
#define PERLIN_DIM 64

layout (set = 0, binding = 0) uniform UBOBuffer
{
	mat4 projection;
	mat4 modelMatrix;
	mat4 viewMatrix;
	vec4 cameraPos;
	vec4 perlinOctave;
	vec4 perlinAmplitude;
	vec2 perlinMovement;
	vec2 dim;
	float dispFactor;
	float tessFactor;
	float tessEdgeSize;
} ubo;

layout (quads, equal_spacing, cw) in;

layout (set = 0, binding = 1) uniform sampler2D dispMap;
layout (set = 0, binding = 3) uniform sampler2D noiseSampler;

layout (location = 0) in vec2 inUv[];
layout (location = 1) in vec3 inPos[];

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec3 outPos;
layout (location = 2) out float outViewLen;

void main()
{
	// interpolate uv
	vec2 u1 = mix(inUv[0], inUv[1], gl_TessCoord.x);
	vec2 u2 = mix(inUv[3], inUv[2], gl_TessCoord.x);
	outUv = mix(u1, u2, gl_TessCoord.y);
	
	// interpolate normals
	//vec3 n1 = mix(inNorm[0], inNorm[1], gl_TessCoord.x);
	//vec3 n2 = mix(inNorm[3], inNorm[2], gl_TessCoord.x);
	//vec3 tempNorm = mix(n1, n2, gl_TessCoord.y);
	
	// interpolated positions from vertex
	vec4 p1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 p2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 pos = mix(p1, p2, gl_TessCoord.y);
	
	// belnding is required to stop artifacts appearing in distant pathces	
	// calculate the local view pos
	//mat4 invModelView = transpose(ubo.viewMatrix * ubo.modelMatrix);
	
	vec3 eyeVec = ubo.cameraPos.xyz - pos.xyz ;
	outViewLen = length(eyeVec);
	
	float blendFactor = (BLENDING_END - outViewLen) / (BLENDING_END - BLENDING_BEGIN);
	blendFactor = clamp(blendFactor, 0.0, 1.0);

	// if distant patch, add noise to reduce tiling artifact
	float noise  = 0.0;
	if(blendFactor < 1) {
	
		vec2 perlinPos = outUv * PERLIN_DIM;
		float perlin_x = textureLod(noiseSampler, perlinPos * ubo.perlinOctave.x * ubo.perlinMovement, 0.0).w;
		float perlin_y = textureLod(noiseSampler, perlinPos * ubo.perlinOctave.y * ubo.perlinMovement, 0.0).w;
		float perlin_z = textureLod(noiseSampler, perlinPos * ubo.perlinOctave.z * ubo.perlinMovement, 0.0).w;
		
		noise = perlin_x * ubo.perlinAmplitude.x + perlin_y * -ubo.perlinAmplitude.y + perlin_z * ubo.perlinAmplitude.z;
	}
		
	// displace the position - derived from map
	vec3 displacement = vec3(0.0);
	if(blendFactor > 0)
		displacement = texture(dispMap, outUv).xyz;
	
	displacement = mix(vec3(0.0, 0.0, noise), displacement, blendFactor);
	pos.x += displacement.x;
	pos.y -= displacement.y;
	pos.z += displacement.z;
	
	// convert everything to world space
	// position (world space)
	outPos = vec3(ubo.modelMatrix * pos).xyz;
	
	gl_Position = ubo.projection * ubo.viewMatrix * ubo.modelMatrix * pos;
}