#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec3 outPos;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	
	gl_Position = vec4(inPos, 1.0);
	
	outPos = inPos;

	outUv = inUv;
}