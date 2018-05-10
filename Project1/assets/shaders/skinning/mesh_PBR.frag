#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 1, binding = 0) uniform sampler2D diffuseMap;
layout (set = 1, binding = 1) uniform sampler2D normalMap;
layout (set = 1, binding = 2) uniform sampler2D roughnessMap;
layout (set = 1, binding = 3) uniform sampler2D metallicMap;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColour;
layout (location = 3) in vec3 inPos;


layout(push_constant) uniform pushConstants 
{
		layout (offset = 16) vec4 colour;
		layout (offset = 32) float roughness;
		layout (offset = 36) float metallic;

} material;

layout (location = 0) out vec4 outColour;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outAlbedo;
layout (location = 4) out float outAo;
layout (location = 5) out float outMetallic;
layout (location = 6) out float outRoughness;

vec3 perturbNormal()
{
	vec3 tangentNormal = texture(normalMap, inUv).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inPos);
	vec3 q2 = dFdy(inPos);
	vec2 st1 = dFdx(inUv);
	vec2 st2 = dFdy(inUv);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main()
{
	//alebdo
	outAlbedo = texture(diffuseMap, inUv) * vec4(inColour, 1.0);
	
	//position
	outPosition = vec4(inPos, 1.0);
	
	// normal
	vec3 norm = perturbNormal();
	outNormal = vec4(norm, 1.0);

	// ao - not used yet
	outAo = 1.0;

	// metallic
	outMetallic = texture(metallicMap, inUv).r;

	// roughness
	outRoughness = texture(roughnessMap, inUv).r;

	outColour = vec4(0.0);
}