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
	vec4 lightPos;
} ubo;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUv;
layout(location = 2) out vec3 outPos;
layout(location = 3) out vec3 outColour;
layout(location = 4) out vec3 outViewMatrix;
layout(location = 5) out vec3 outLightPos;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	mat4 modelView = ubo.viewMatrix * ubo.modelMatrix;
	
	gl_Position = ubo.projection * modelView * vec4(inPos.xyz, 1.0);
	
	vec4 pos = modelView * vec4(inPos.xyz, 1.0);

	vec3 lightP = mat3(ubo.modelMatrix) * ubo.lightPos.xyz;
	outLightPos = lightP - (ubo.modelMatrix * vec4(inPos, 1.0)).xyz;
	outViewMatrix = -(ubo.modelMatrix * vec4(inPos, 1.0)).xyz;
	
	outPos = inPos;
	outUv = inUv;
	outColour = inColour;
	outNormal = mat3(ubo.modelMatrix) * inNormal;
}