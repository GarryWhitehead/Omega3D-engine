#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 1) uniform sampler2D normalMap;
layout (set = 0, binding = 2) uniform sampler2D colorTexture;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inPos;

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
	// output fragment information to the appropiate buffers - lighting will be calculated in the deferred pass	
	outAlbedo.rgb = texture(colorTexture, inUv);	

	outPosition = vec4(inPos, 1.0);
	
	outNormal = vec4(texture(normalMap, inUv).xyz, 3.0);	// w indicates the deferred shader that this frag is water

	outBump = vec4(128.0, 128.0, 256.0, 1.0);

	outAo = 0.0;
	outMetallic.r = 0.0;
	outRoughness.r = 0.0;

	outColour = vec4(0.0);
}