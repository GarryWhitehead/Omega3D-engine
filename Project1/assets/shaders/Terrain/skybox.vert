#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	vec4 offsets[3];
} ubo;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outUv;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	vec4 tempPos = vec4(inPos, 1.0) + ubo.offsets[gl_InstanceIndex];

	gl_Position = ubo.projection * ubo.model * tempPos;
	
	outUv = inPos;
	outUv.x *= -1.0;
	outUv.t = 1.0 - outUv.t;
}