#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform samplerCube samplerMap;

layout (location = 0) in vec3 inUv;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

void main() 
{
	outPosition = vec4(inUv, 1.0);
	
	outNormal = vec4(1.0);		// not used yet
	
	outAlbedo = texture(samplerMap, inUv);
}