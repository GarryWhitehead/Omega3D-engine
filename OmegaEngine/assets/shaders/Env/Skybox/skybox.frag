#version 450

layout (binding = 1) uniform samplerCube samplerMap;

layout (location = 0) in vec3 inUv;

layout (location = 0) out vec4 outCol;

void main() 
{	
	outCol = textureLod(samplerMap, inUv, 0.5);
}