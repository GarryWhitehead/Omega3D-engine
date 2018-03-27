#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


//shadow sampler
//layout (set = 0, binding = 1) uniform sampler2D shadowMap;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUv;				// not used in the no-material version
layout(location = 2) in vec3 inPos;
layout(location = 3) in vec3 inColour;
layout(location = 4) in vec3 inViewMatrix;
layout(location = 5) in vec3 inLightDir;
layout(location = 6) in vec4 inShadowPos;

layout(location = 0) out vec4 outFragColour;
	
//float TextureCrop(vec4 P, vec2 offset)
//{
//	float shadow = 1.0;
//	float bias = 0.005;
//	
//	vec4 shadowPos = P / P.w;
//	if(shadowPos.z > -1.0 && shadowPos.z < 1.0) {
//	
//		float dist = texture(shadowMap, shadowPos.st + offset).r;
//		if(shadowPos.w > 0 && dist < shadowPos.z - bias) {
//		
//			shadow = 0.3;		// ambient
//		}
//	}
//	return shadow;
//}

void main()
{	
	//float shadow = TextureCrop(inShadowPos / inShadowPos.w, vec2(0.0));
	
	#define ambient 0.3
	
	// diffuse
	vec3 N = normalize(inNormal);
	vec3 L = normalize(-inLightDir);
	vec3 R = normalize(-reflect(L, N));
	vec3 diffuse = max(dot(N, L), 0.3) * inColour;
	
	outFragColour = vec4(diffuse, 1.0);

}
	
	