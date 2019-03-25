#version 450

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUv;
layout (location = 2) in vec3 inNormal;

layout (set = 1, binding = 0) uniform CameraUbo
{
	mat4 mvp;

} camera_ubo;

layout (set = 2, binding = 0) uniform Dynamic_StaticMeshUbo
{
	mat4 modelMatrix;
} mesh_ubo;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outPos;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{	
	mat4 normalTransform = mesh_ubo.modelMatrix;
	vec4 pos = normalTransform * inPos;
	outNormal = (normalTransform * vec4(inNormal, 1.0)).xyz;

	pos = -pos;
	outPos = pos.xyz / pos.w;	// perspective divide correction
	
	gl_Position = camera_ubo.mvp * vec4(outPos, 1.0);
	outUv = inUv;
}