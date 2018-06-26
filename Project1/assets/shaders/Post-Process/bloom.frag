#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outFrag;

layout (push_constant) uniform pushConstant
{
	uint direction;		// shader can either work in the horizontal or vertical direction
} push;

layout (binding = 1) uniform uboBuffer
{
	float blurStrength;
	float blurScale;
};

layout (binding = 2) uniform sampler2D colSampler;		// the image containing pixels above the brightness threshold

const float weight[] = float[]
(
	0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216
);

void main()
{
	vec2 texOffset = 1.0 / textureSize(colSampler, 0) * blurScale;
	vec3 result = texture(colSampler, inUv).rgb * weight[0];
	
	for(int i = 1; i < weight.length(); i++) {
	
		if(push.direction == 1) {		// horizontal
			result += texture(colSampler, inUv + vec2(texOffset.x * i, 0.0)).rgb * weight[i] * blurStrength; 
			result += texture(colSampler, inUv - vec2(texOffset.x * i, 0.0)).rgb * weight[i] * blurStrength; 
		}
		else {							// vertical
			result += texture(colSampler, inUv + vec2(0.0, texOffset.y * i)).rgb * weight[i] * blurStrength; 
			result += texture(colSampler, inUv - vec2(0.0, texOffset.y * i)).rgb * weight[i] * blurStrength; 
		}
	}
	outFrag = vec4(result, 1.0);
}