#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 outFragCol;
layout (location = 1) out vec4 outBrightCol;

layout (location = 0) in vec2 inUv;

layout (binding = 1) uniform sampler2D hdrSampler;

#define THRESHOLD 0.75

void main()
{
	outFragCol = texture(hdrSampler, inUv);
	
	float brightness = dot(outFragCol.rgb, vec3(0.2126, 0.7152, 0.0722));		// hard-coded brightness threshold - change to spec constant
	
	if(brightness > THRESHOLD)
		outBrightCol = vec4(outFragCol.rgb, 1.0);		// if frag colour is greater than threshhold, render to bright sampler for blurring
	else
		outBrightCol = vec4(0.0, 0.0, 0.0, 1.0);
}