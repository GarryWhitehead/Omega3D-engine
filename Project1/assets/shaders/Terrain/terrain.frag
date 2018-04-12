#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D heightSampler;
layout (set = 0, binding = 2) uniform sampler2DArray textureArray;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inPos;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;
	
const mat4 cropMat = mat4
(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0 ,0.0,
	0.0, 0.0, 0.5, 0.0,
	0.5, 0.5, 0.0, 1.0
);
	
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
	
	for(int i = 0; i < 5; i++) {
	
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
	// output fragment information to the appropiate buffers - lighting will be calculated in the deferred pass	
	outAlbedo = getTerrainLayer();
	
	outPosition = vec4(inPos, 1.0);
	
	outNormal = vec4(normalize(inNormal), 0.0);
}