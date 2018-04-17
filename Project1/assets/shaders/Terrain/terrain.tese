#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform UBOBuffer
{
	mat4 projection;
	mat4 modelMatrix;
	mat4 viewMatrix;
	vec2 dim;
	float dispFactor;
	float tessFactor;
	float tessEdgeSize;
} ubo;

layout (quads, equal_spacing, cw) in;

layout (set = 0, binding = 1) uniform sampler2D dispMap;

layout (location = 0) in vec2 inUv[];
layout (location = 1) in vec3 inNorm[];
layout (location = 2) in vec3 inPos[];

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec3 outNorm;
layout (location = 2) out vec3 outPos;

void main()
{
	// interpolate uv
	vec2 u1 = mix(inUv[0], inUv[1], gl_TessCoord.x);
	vec2 u2 = mix(inUv[3], inUv[2], gl_TessCoord.x);
	outUv = mix(u1, u2, gl_TessCoord.y);
	
	// interpolate normals
	vec3 n1 = mix(inNorm[0], inNorm[1], gl_TessCoord.x);
	vec3 n2 = mix(inNorm[3], inNorm[2], gl_TessCoord.x);
	vec3 tempNorm = mix(n1, n2, gl_TessCoord.y);
	
	// interpolated positions from vertex
	vec4 p1 = mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x);
	vec4 p2 = mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x);
	vec4 pos = mix(p1, p2, gl_TessCoord.y);
	
	// displace the y coord depending on height derived from map
	pos.y -= textureLod(dispMap, outUv, 0.0).r * ubo.dispFactor;
	
	// convert everything to world space
	
	gl_Position = ubo.projection * ubo.viewMatrix * ubo.modelMatrix * pos;
	
	// position (world space)
	outPos = vec3(ubo.modelMatrix * pos);
	
	outUv.t = 1.0 - outUv.t;
	
	// normal (world space)
	mat3 mNorm = transpose(inverse(mat3(ubo.modelMatrix)));
	outNorm = mNorm * normalize(tempNorm);
}