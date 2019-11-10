#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(binding = 1) uniform fogData
{			 
	float rayDir;
	float sunDir;				
	float fogDensity;			// try 0.05
	uint enableFog;
};
 
layout (set = 0, binding = 2) uniform sampler2D colorSampler;
 
layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inViewSpace;
 
layout(location = 0) out vec4 outFrag;
 
void main()
{
 
	vec3 pixelColour = texture(colorSampler, inUV).rgb;
	
	if(enableFog == 1) {
		
		// exponential fog equation - f = exp-(density * dist(density))
		float dist = length(inViewSpace);	 
		float fogAmount = 1.0 / exp(dist * fogDensity);
		fogAmount = clamp(fogAmount, 0.0, 1.0);
	
		float sunAmount = max(dot(rayDir, sunDir), 0.0);	
	
		vec3  fogColor  = mix(vec3(0.5,0.6,0.7), vec3(1.0,0.9,0.7), pow(sunAmount,8.0)); // blueish, yellowish tint
		vec3 finalColour = mix(fogColor, pixelColour, fogAmount);
	 
		//vec3 extColor = vec3( exp(-distance*be.x), exp(-distance*be.y) exp(-distance*be.z) );
		//vec3 insColor = vec3( exp(-distance*bi.x), exp(-distance*bi.y) exp(-distance*bi.z) );
		//finalColor = pixelColor*(1.0-extColor) + fogColor*insColor;

		outFrag = vec4(finalColour, 1.0);
	}
	else {
		outFrag = vec4(pixelColour, 1.0);
	}
}