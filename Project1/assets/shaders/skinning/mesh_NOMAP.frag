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
		vec4 ambient;
		vec4 diffuse;
		vec4 specular;
		vec4 color;
		float shininess;
		float transparency;
} material;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;

void main()
{
	outAlbedo = material.diffuse * vec4(inColour, 1.0);
	
	outPosition = vec4(inPos, 1.0);
	
	outNormal = vec4(inNormal, 1.0);
}