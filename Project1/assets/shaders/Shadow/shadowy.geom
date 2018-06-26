#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 0) buffer SsboBuffer1
{
	mat4 modelMatrix[256];
} ssbo;

layout (binding = 1) buffer Ssbobuffer2
{
	mat4 mvp[256];
} ubo;

layout (push_constant) uniform entitiyIndex
{
	uint useModelIndex;
	uint entIndex;
} push;

layout (triangles, invocations = 5) in;
layout (triangle_strip, max_vertices = 3) out;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	for(int i = 0; i < gl_in.length(); i++) {
	
		gl_Layer = gl_InvocationID;
		
		if(push.useModelIndex == 1) {
			gl_Position = ubo.mvp[gl_InvocationID] * ssbo.modelMatrix[push.entIndex] * gl_in[i].gl_Position;
		}
		else if(push.useModelIndex == 0) {
			gl_Position = ubo.mvp[gl_InvocationID] * gl_in[i].gl_Position;
		}
		EmitVertex();
	}
	EndPrimitive();
}
