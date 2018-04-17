#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define LIGHT_COUNT 3

layout (triangles, invocations = LIGHT_COUNT) in;
layout (triangle_strip, max_vertices = 3) out;

layout (binding = 0) uniform UBObuffer
{
	mat4 mvp[LIGHT_COUNT];
	vec4 instancePos[3];
} ubo;

layout (location = 0) in int inIndex[];

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	vec4 indexedPos = ubo.instancePos[inIndex[0]];
	for(int i = 0; i < gl_in.length(); i++) {
	
		gl_Layer = gl_InvocationID;
		//vec4 tempPos = gl_in[i].gl_Position + indexedPos;
		gl_Position = ubo.mvp[gl_InvocationID] * gl_in[i].gl_Position;
		EmitVertex();
	}
	EndPrimitive();
}
