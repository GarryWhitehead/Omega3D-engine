#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D heightSampler;
layout (set = 0, binding = 2) uniform sampler2DArray textureArray;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inPos;
layout (location = 3) in vec3 inViewPos;

layout (location = 0) out vec4 outFragColour;
	
const mat4 cropMat = mat4
(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0 ,0.0,
	0.0, 0.0, 0.5, 0.0,
	0.5, 0.5, 0.0, 1.0
);

float TextureCrop(vec4 P, vec2 offset)
{
	float shadow = 1.0;
	float bias = 0.005;
	
	vec4 shadowPos = P / P.w;
	if(shadowPos.z > -1.0 && shadowPos.z < 1.0) {
	
		float dist = texture(textureArray, vec3(shadowPos.st + offset, 1.0)).r;
		if(shadowPos.w > 0 && dist < shadowPos.z - bias) {
		
			shadow = 0.3;		// ambient
		}
	}
	return shadow;
}
		
vec4 getTerrainLayer()
{
	// procedural texture generation using a texture array
	vec2 layer[5];
	layer[0] = vec2(-10.0, 50.0);	//	water
	layer[1] = vec2(40.0, 85.0);	// dirt
	layer[2] = vec2(75.0, 120.0);	// grass
	layer[3] = vec2(120.0, 190.0);	// stone
	layer[4] = vec2(180.0, 255.0);	// snow
	
	float height = textureLod(heightSampler, inUv, 0.0).r * 255.0;	// convert red into greyscale (0-256)
	
	vec4 finalColour = vec4(0.0);
	
	for(int i = 0; i < 6; i++) {
	
		float range = layer[i].y - layer[i].x;
		float weight = (range - abs(height - layer[i].y)) / range;
		weight = max(0.0, weight);									// if less than zero, return zero
		vec4 color = texture(textureArray, vec3(inUv * 16.0, i));
		finalColour.rgb += weight * color.rgb;
		finalColour.a = color.a;
	}
	
	return finalColour;
}

void main()
{
	//vec4 shadowPos = (cropMat * shadowUbo.viewProjMat[index]) * vec4(inPos, 1.0);
	//float shadow = TextureCrop(shadowPos / shadowPos.w, vec2(0.0), index);
	
	// lighting
	// ambient
	vec3 ambient = vec3(0.5, 0.5, 0.5);
	
	vec4 color = getTerrainLayer();
	
	// diffuse
	vec3 N = normalize(inNormal);
	vec3 L = normalize(-vec3(-48.0f, -48.0f, 46.0f));
	vec3 H = normalize(L + inViewPos);
	float diffuse = max(dot(N, L), 0.3);
	vec3 lightColour = vec3(1.0);
	
	outFragColour = vec4(diffuse * color.rgb, 1.0);
	//outFragColour *= shadow;
}