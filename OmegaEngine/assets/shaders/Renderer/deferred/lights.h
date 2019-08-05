#ifndef LIGHTS_H
#define LIGHTS_H

struct SpotLight
{
		mat4 viewMatrix;
		vec4 pos;
		vec4 direction;
		vec4 colour;
		float scale;
		float offset;
		float fallOut;
};

struct PointLight
{
		mat4 viewMatrix;
		vec4 pos;
		vec4 colour;
		float fallOut;
};

struct DirectionalLight
{
		mat4 viewMatrix;
		vec4 pos;
		vec4 colour;
};

float calculateAngle(vec3 lightDir, vec3 L, float scale, float offset)
{
	float angle = dot(lightDir, L);
	float attenuation = clamp(angle * scale + offset, 0.0, 1.0);
	return attenuation * attenuation;
}

float calculateDistance(vec3 L, float fallOut)
{
	float dist = dot(L, L);
	float factor = dist * fallOut;
	float smoothFactor = clamp(1.0 - factor * factor, 0.0, 1.0);
	float smoothFactor2 = smoothFactor * smoothFactor;
	return smoothFactor2 * 1.0 / max(dist, 1e-4);
}

#endif

	