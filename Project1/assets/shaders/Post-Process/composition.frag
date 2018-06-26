#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 1) uniform uboBuffer			
{			 
	float rayDir;
	float sunDir;				
	float fogDensity;			// try 0.05
	float expBias;
	float gamma;
	float _pad0;
	uint enableFog;
} ubo;
 
layout (set = 0, binding = 2) uniform sampler2D sceneImage;
layout (set = 0, binding = 3) uniform sampler2D bloomImage;
 
layout(location = 0) in vec2 inUv;
layout(location = 1) in vec4 inViewSpace;
 
layout(location = 0) out vec4 outFrag;

#define BLUR_LEVELS 4

vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
	// begin by merging the blurred images from each mip level
	vec3 blurCol = vec3(0.0);
	//for(uint c = 0; c < BLUR_LEVELS; c++)
	blurCol += texture(bloomImage, inUv).rgb;
		
	// now blend scene colour with bloom
	vec3 finalColour = texture(sceneImage, inUv).rgb;
	finalColour += blurCol;
	
	// tone mapping - from http://filmicworlds.com/blog/filmic-tonemapping-operators/
	finalColour = Uncharted2Tonemap(ubo.expBias * finalColour);
	
	vec3 whiteScale = vec3(1.0 / Uncharted2Tonemap(vec3(11.2)));
	finalColour *= whiteScale;
	finalColour = pow(finalColour, vec3(1.0/ubo.gamma));		// to the power of 1/gamma
	
	if(ubo.enableFog == 1) {
		
		// exponential fog equation - f = exp-(density * dist(density))
		float dist = length(inViewSpace);	 
		float fogAmount = 1.0 / exp(dist * ubo.fogDensity);
		fogAmount = clamp(fogAmount, 0.0, 1.0);
	
		float sunAmount = max(dot(ubo.rayDir, ubo.sunDir), 0.0);	
	
		vec3  fogColor  = mix(vec3(0.5,0.6,0.7), vec3(1.0,0.9,0.7), pow(sunAmount,8.0)); // blueish, yellowish tint
		vec3 finalColour = mix(fogColor, finalColour, fogAmount);
	 
		//vec3 extColor = vec3( exp(-distance*be.x), exp(-distance*be.y) exp(-distance*be.z) );
		//vec3 insColor = vec3( exp(-distance*bi.x), exp(-distance*bi.y) exp(-distance*bi.z) );
		//finalColor = pixelColor*(1.0-extColor) + fogColor*insColor;
	}
	
	outFrag = vec4(finalColour, 1.0);
}