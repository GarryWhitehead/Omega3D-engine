#version 450

layout (binding = 1) uniform samplerCube SkyboxSampler;

layout (location = 0) in vec3 inUv;

layout (location = 0) out vec4 outCol;

layout(push_constant) uniform pushConstants
{
	float blurFactor;
} push;

void main() 
{	
	outCol = textureLod(SkyboxSampler, inUv, push.blurFactor);
}