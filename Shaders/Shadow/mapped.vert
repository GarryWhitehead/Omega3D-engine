#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUv0;		// not used
layout (location = 2) in vec2 inUv1;		// not used
layout (location = 3) in vec3 inNormal;		// not used

layout (set = 0, binding = 0) uniform Dynamic_Ubo
{
	mat4 lightMvp;
} ubo;

void main()
{
	gl_Position = ubo.lightMvp * inPos;
}
