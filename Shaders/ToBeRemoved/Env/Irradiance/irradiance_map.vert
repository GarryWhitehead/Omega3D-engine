#version 450

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec3 outUv;

layout (push_constant) uniform PushConstant
{
	mat4 mvp;
} push;

void main()
{
	outUv = inPos;
	
	gl_Position = push.mvp * vec4(inPos.xyz, 1.0);
}