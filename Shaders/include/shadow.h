#ifndef SHADOW_H
#define SHADOW_H

// shadow projection
float textureProj(vec4 P, vec2 offset, sampler2D shadowSampler)
{
	const float shadowFactor = 0.1f;
	
	float shadow = 1.0;
	vec4 shadowCoord = P / P.w;
	shadowCoord.st = shadowCoord.st * 0.5 + 0.5;
	
	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) 
	{
		float dist = texture(shadowSampler, vec2(shadowCoord.st + offset)).r;
		if (shadowCoord.w > 0.0 && dist < shadowCoord.z) 
		{
			shadow = shadowFactor;
		}
	}
	return shadow;
}

// shadow filter PCF
float shadowPCF(vec4 shadowCoord, sampler2D shadowSampler)
{
	ivec2 dim = textureSize(shadowSampler, 0).xy;
	float scale = 1.5;
	float dx = scale * 1.0 / float(dim.x);
	float dy = scale * 1.0 / float(dim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;

	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(shadowCoord, vec2(dx * x, dy * y), shadowSampler);
			count++;
		}
	}

	return shadowFactor / count;
}

#endif