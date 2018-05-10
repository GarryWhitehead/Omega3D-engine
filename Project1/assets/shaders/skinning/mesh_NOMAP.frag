#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 1, binding = 0) uniform sampler2D diffuseMap;		// not used 

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColour;
layout (location = 3) in vec3 inPos;


layout(push_constant) uniform pushConstants 
{
		layout (offset = 16) vec4 colour;
		layout (offset = 32) float roughness;
		layout (offset = 36) float metallic;

} material;

layout (location = 0) out vec4 outColour;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outAlbedo;
layout (location = 4) out float outAo;
layout (location = 5) out float outMetallic;
layout (location = 6) out float outRoughness;


void main()
{
	outAlbedo = vec4(inColour, 1.0);
	
	outPosition = vec4(inPos, 1.0);
	
	outNormal = vec4(inNormal, 1.0);

	outAo = 1.0;

	outMetallic.r = material.metallic;
	outRoughness.r = material.roughness;

	outColour = vec4(0.0);
}