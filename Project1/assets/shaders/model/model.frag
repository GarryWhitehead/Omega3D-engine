#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D diffuseMap;
layout (set = 0, binding = 2) uniform sampler2D normalMap;
layout (set = 0, binding = 3) uniform sampler2D roughnessMap;
layout (set = 0, binding = 4) uniform sampler2D metallicMap;
layout (set = 0, binding = 5) uniform sampler2D aoMap;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColour;
layout (location = 3) in vec3 inPos;

layout(push_constant) uniform pushConstants 
{
		layout(offset = 4) float roughness;
		layout(offset = 8) float metallic;
		layout(offset = 12) float ao;
		layout(offset = 16) float r;
		layout(offset = 20) float g;
		layout(offset = 24) float b;

} material;

layout (location = 0) out vec4 outColour;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outAlbedo;
layout (location = 4) out vec4 outBump;
layout (location = 5) out float outAo;
layout (location = 6) out float outMetallic;
layout (location = 7) out float outRoughness;

void main()
{
	//alebdo
	outAlbedo = texture(diffuseMap, inUv) * vec4(inColour, 1.0);
	
	//position
	outPosition = vec4(inPos, 1.0);
	
	// normal
	outNormal = vec4(inNormal, 1.0);

	// bump
	outBump = texture(normalMap, inUv);
	
	// ao 
	outAo = texture(aoMap, inUv).r;

	// metallic
	outMetallic = texture(metallicMap, inUv).r;

	// roughness
	outRoughness = texture(roughnessMap, inUv).r;

	// needed due to a bug that will cause undefined behaviour
	outColour = vec4(0.0);
}