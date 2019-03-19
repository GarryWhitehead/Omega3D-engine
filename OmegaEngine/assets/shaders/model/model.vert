#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inColour;
layout (location = 4) in vec4 inWeights;
layout (location = 5) in ivec4 inBoneId;

#define MAX_BONES 64

layout (set = 1, binding = 0) uniform CameraUbo
{
	mat4 mvp;

} camera_ubo;

layout (set = 2, binding = 0) uniform Dynamic_StaticMeshUbo
{
	mat4 modelMatrix;
} mesh_ubo;

layout (set = 3, binding = 0) uniform Dynamic_SkinnedUbo
{
	mat4 matrix;
	mat4 bones[MAX_BONES];
	float jointCount;
} skinned_ubo;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec4 outTangent;
layout (location = 3) out vec3 outColour;
layout (location = 4) out vec3 outPos;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{	
	vec4 pos;
	
	if (skinned_ubo.jointCount > 0.0) {
		mat4 boneTransform = skinned_ubo.bones[inBoneId[0]] * inWeights[0];
		boneTransform += skinned_ubo.bones[inBoneId[1]] * inWeights[1];
		boneTransform += skinned_ubo.bones[inBoneId[2]] * inWeights[2];
		boneTransform += skinned_ubo.bones[inBoneId[3]] * inWeights[3];
		
		mat4 normalTransform = mesh_ubo.modelMatrix * skinned_ubo.matrix * boneTransform;
		pos = normalTransform * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(normalTransform))) * inNormal);
	}
	else {
		mat4 normalTransform = mesh_ubo.modelMatrix * skinned_ubo.matrix;
		pos = normalTransform * vec4(inPos, 1.0);
		outNormal = normalize(transpose(inverse(mat3(normalTransform))) * inNormal);
	}

	pos = -pos;
	outPos = pos.xyz / pos.w;	// perspective divide
	
	gl_Position = camera_ubo.mvp * vec4(outPos, 1.0);
	outUv = inUv;
	outColour = inColour;
}