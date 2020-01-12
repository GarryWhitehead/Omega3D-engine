#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec4 inCol;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec4 outCol;

layout (push_constant) uniform pushConstant
{
	vec2 scale;
} push;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	outCol = inCol;
	outUv = inUv;
	
	gl_Position = vec4(inPos * push.scale + vec2(-1.0), 0.0, 1.0);
}
