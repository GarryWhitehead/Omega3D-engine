#version 450

layout (location = 0) in vec3 inPos;

layout (location = 0) out vec3 outUv;

layout (push_constant) uniform PushConstant
{
	layout (offset = 0) mat4 mvp;
} push;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main()
{
	outUv = inPos;
	
	gl_Position = push.mvp * vec4(inPos.xyz, 1.0);
}