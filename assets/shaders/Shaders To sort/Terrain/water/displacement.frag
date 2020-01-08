#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outDisplace;

layout (binding = 0) uniform uboBuffer
{
	float choppiness;
	uint mapSize;
} ubo;

layout (binding = 1) buffer ssboBufferdx
{
	vec2 dxtMap[];
};

layout (binding = 2) buffer ssboBuffer2dy
{
	vec2 dytMap[];
};

layout (binding = 3) buffer ssboBuffer3dz
{
	vec2 htMap[];
};

void main()
{

	uint xIndex = uint(inUv.x) * ubo.mapSize;
	uint yIndex = uint(inUv.y) * ubo.mapSize;
	uint index = ubo.mapSize * yIndex + xIndex;
	
	// odds = -1 / evens +1
	uint res = ((xIndex + yIndex) & 1);
	uint signCorrect = (res == 0) ? -1 : 1;
	
	// update displacement map
	vec3 displace;
	displace.x = dxtMap[index].x * signCorrect * ubo.choppiness;
	displace.y = dytMap[index].x * signCorrect * ubo.choppiness;
	displace.z = htMap[index].x * signCorrect;
	
	outDisplace = vec4(displace, 1.0);
}