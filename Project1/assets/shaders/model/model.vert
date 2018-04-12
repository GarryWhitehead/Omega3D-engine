#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inColour;

layout(set = 0, binding = 0) uniform UBOBuffer
{
	mat4 projection;
	mat4 viewMatrix;
	mat4 modelMatrix;
	vec4 offsets[3];
} ubo;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUv;
layout(location = 2) out vec3 outPos;
layout(location = 3) out vec3 outColour;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	gl_Position = ubo.projection * ubo.viewMatrix * ubo.modelMatrix * vec4(inPos, 1.0);
	
	outPos = vec3(ubo.modelMatrix * vec4(inPos, 1.0)).xyz;
	
	outUv = inUv;
	outUv.t = 1.0 - outUv.t;

	outColour = inColour;
	
	mat3 mNorm = transpose(inverse(mat3(ubo.modelMatrix)));
	outNormal = mNorm * normalize(inNormal);
}