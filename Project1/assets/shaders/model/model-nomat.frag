#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUv;				// not used in the no-material version
layout(location = 2) in vec3 inPos;
layout(location = 3) in vec3 inColour;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;
	
void main()
{	
	outPosition = vec4(inPos, 1.0);
	
	outNormal = vec4(inNormal, 0.0);
	
	outAlbedo = vec4(inColour, 1.0);
}
	
	