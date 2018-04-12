#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;

layout (binding = 0) uniform UboBuffer
{
	mat4 projection;
	mat4 viewMatrix;
	mat4 modelMatrix;
} ubo;

layout (location = 0) out vec2 outUv;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	outUv = inUv;
	gl_Position = ubo.projection * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos, 1.0);
}