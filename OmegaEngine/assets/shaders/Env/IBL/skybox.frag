#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform samplerCube samplerMap;

layout (location = 0) in vec3 inUv;

layout (location = 0) out vec4 outCol;

void main() 
{	
	outCol = texture(samplerMap, inUv);
}