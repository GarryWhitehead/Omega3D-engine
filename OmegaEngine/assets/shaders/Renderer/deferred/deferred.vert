#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;

layout (set = 0, binding = 0) uniform UboBuffer
{
	mat4 mvp;
} ubo;

layout (location = 0) out vec2 outUv;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	outUv = inUv;
	gl_Position = ubo.mvp * vec4(inPos, 1.0);
}