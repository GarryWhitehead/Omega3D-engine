#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D displacementMap;

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outNormal;

layout (binding = 0) uniform uboBuffer
{
	float choppiness;
	float gridLength;
} ubo;

void main()
{
	vec2 texelSize = 1.0 / vec2(textureSize(displacementMap, 0));
	
	vec2 offsetLeft = vec2(inUv.x - texelSize.x, inUv.y);
	vec2 offsetRight = vec2(inUv.x + texelSize.x, inUv.y);
	vec2 offsetFront = vec2(inUv.x, inUv.y - texelSize.y);
	vec2 offsetBack = vec2(inUv.x, inUv.y + texelSize.y);
	
	vec3 disLeft = texture(displacementMap, offsetLeft).xyz;
	vec3 disRight = texture(displacementMap, offsetRight).xyz;
	vec3 disFront= texture(displacementMap, offsetFront).xyz;
	vec3 disBack = texture(displacementMap, offsetBack).xyz;
	
	vec2 grad = vec2(-(disRight.z - disLeft.z), -(disFront.z - disBack.z));
	
	// calculate Jacobian correlation and store in w component of output vec
	vec2 dx = vec2(disRight.xy - disLeft.xy) * ubo.choppiness * ubo.gridLength;
	vec2 dy = vec2(disFront.xy - disBack.xy) * ubo.choppiness * ubo.gridLength;
	
	float j = (1.0 + dx.x) * (1.0 + dy.y) - dx.y * dy.x;
	
	outNormal = vec4(grad, 0.0, max(1.0 - j, 0));
}