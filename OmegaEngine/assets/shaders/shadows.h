
float textureProj(vec4 P, float layer, vec2 offset)
{
	float shadow = 1.0;
	vec4 shadowCoord = P / P.w;
	shadowCoord.st = shadowCoord.st * 0.5 + 0.5;
	
	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) 
	{
		float dist = texture(shadowSampler, vec3(shadowCoord.st + offset, layer)).r;
		if (shadowCoord.w > 0.0 && dist < shadowCoord.z) 
		{
			shadow = SHADOW_FACTOR;
		}
	}
	return shadow;
}

vec3 shadowLight()
{
	// shadow calculations
		
		for(int i = 0; i < push.activeLightCount; i++) {
		
			vec4 shadowClip	= ubo.lights[i].viewMatrix * vec4(inPos, 1.0);
			float shadowFactor = textureProj(shadowClip, i, vec2(0.0));
			
			finalColour *= shadowFactor;
		}
}