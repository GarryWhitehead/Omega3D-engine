#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec4 inCol;

layout (location = 0) out vec4 outFrag;

layout (set = 0, binding = 0) uniform sampler2D fontSampler;

void main()
{
	outFrag = texture(fontSampler, inUv) * inCol;
}
