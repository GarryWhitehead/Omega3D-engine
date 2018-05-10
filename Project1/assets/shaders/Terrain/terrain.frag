#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D heightSampler;
layout (set = 0, binding = 2) uniform sampler2DArray textureArray;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inPos;

layout (location = 0) out vec4 outColour;
layout (location = 1) out vec4 outPosition;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec4 outAlbedo;
layout (location = 4) out float outAo;
layout (location = 5) out float outMetallic;
layout (location = 6) out float outRoughness;


vec3 getTerrainLayer()
{
	// procedural texture generation using a texture array
	vec2 layer[6];
	layer[0] = vec2(-10.0, 10.0);	// water
	layer[1] = vec2(5.0, 45.0);	// dirt
	layer[2] = vec2(45.0, 80.0);	// grass
	layer[3] = vec2(75.0, 100.0);	// stone
	layer[4] = vec2(95.0, 140.0);	// snow
	layer[5] = vec2(140.0, 190.0);	// snow
	
	float height = textureLod(heightSampler, inUv, 0.0).r * 255.0;	// convert red into greyscale (0-256)
	
	vec3 finalColour = vec3(0.0);
	
	for(int i = 0; i < 6; i++) {
	
		float range = layer[i].y - layer[i].x;
		float weight = (range - abs(height - layer[i].y)) / range;
		weight = max(0.0, weight);					
		vec4 color = texture(textureArray, vec3(inUv * 16.0, i));
		finalColour += weight * color.rgb;
	}
	
	return finalColour;
}

void main()
{	
	// output fragment information to the appropiate buffers - lighting will be calculated in the deferred pass	
	outAlbedo.rgb = getTerrainLayer();	

	outPosition = vec4(inPos, 1.0);
	
	outNormal = vec4(inNormal, 1.0);

	outAo = 1.0;
	outMetallic.r = 0.8;
	outRoughness.r = 0.8;

	outColour = vec4(0.0);
}