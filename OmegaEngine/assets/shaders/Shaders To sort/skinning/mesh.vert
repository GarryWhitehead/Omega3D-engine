#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inColour;
layout (location = 4) in vec4 inWeights;
layout (location = 5) in ivec4 inBoneId;

#define MAX_BONES 64

layout (binding = 0) uniform UBOBuffer
{
	mat4 projection;
	mat4 viewMatrix;
	mat4 modelMatrix;
	mat4 bones[MAX_BONES];
} ubo;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outColour;
layout (location = 3) out vec3 outPos;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{	
	mat4 boneTransform = ubo.bones[inBoneId[0]] * inWeights[0];
	boneTransform += ubo.bones[inBoneId[1]] * inWeights[1];
	boneTransform += ubo.bones[inBoneId[2]] * inWeights[2];
	boneTransform += ubo.bones[inBoneId[3]] * inWeights[3];
	
	gl_Position = ubo.projection * ubo.viewMatrix * ubo.modelMatrix * boneTransform * vec4(inPos, 1.0);

	outPos = vec3(ubo.modelMatrix * vec4(inPos, 1.0)).xyz;
	
	outUv = inUv;
	outUv.t = 1.0 - outUv.t;

	outColour = inColour;
	
	mat3 mNorm = transpose(inverse(mat3(ubo.modelMatrix)));
	outNormal = mNorm * normalize(inNormal);
}