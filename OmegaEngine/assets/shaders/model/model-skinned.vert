#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUv0;
layout (location = 2) in vec2 inUv1;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec4 inWeights;
layout (location = 5) in vec4 inBoneId;

#define MAX_BONES 6

layout (set = 0, binding = 0) uniform CameraUbo
{
	mat4 mvp;

} camera_ubo;

layout (set = 1, binding = 0) uniform Dynamic_StaticMeshUbo
{
	mat4 modelMatrix;
} mesh_ubo;

layout (set = 1, binding = 1) uniform Dynamic_SkinnedUbo
{
	mat4 bones[MAX_BONES];
	float jointCount;
} skinned_ubo;

layout (location = 0) out vec2 outUv0;
layout (location = 1) out vec2 outUv1;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outPos;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{	
	vec4 pos;
	
	mat4 boneTransform = skinned_ubo.bones[int(inBoneId.x)] * inWeights.x;
	boneTransform += skinned_ubo.bones[int(inBoneId.y)] * inWeights.y;
	boneTransform += skinned_ubo.bones[int(inBoneId.z)] * inWeights.z;
	boneTransform += skinned_ubo.bones[int(inBoneId.w)] * inWeights.w;
		
	mat4 normalTransform = mesh_ubo.modelMatrix * boneTransform;
	pos = normalTransform * inPos;

    // inverse-transpose for non-uniform scaling - expensive computations here - maybe remove this?
	outNormal = normalize(transpose(inverse(mat3(normalTransform))) * inNormal);    // 

	pos = -pos;
	outPos = pos.xyz / pos.w;	// perspective divide
	
	gl_Position = camera_ubo.mvp * vec4(outPos, 1.0);
	outUv0 = inUv0;
	outUv1 = inUv1;
}