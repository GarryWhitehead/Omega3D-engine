#ifndef PBR_H
#define PBR_H

#define PI 3.1415926535897932384626433832795

float GGX_Distribution(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	
	return (a2)/(PI * denom * denom); 
}

float GeometryShlickGGX(float NdotV, float NdotL, float roughness)
{
	float k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;
	float GV = NdotV / (NdotV * (1.0 - k) + k);
	float GL = NdotL / (NdotL * (1.0 - k) + k);
	return GL * GV;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 FresnelRoughness(float cosTheta, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 specularContribution(vec3 L, vec3 V, vec3 N, vec3 F0, float metallic, float roughness, vec3 albedo, vec3 radiance)
{
	vec3 H = normalize(V + L);
	float NdotH = clamp(dot(N, H), 0.0, 1.0);
	float NdotV = clamp(dot(N, V), 0.0, 1.0);
	float NdotL = clamp(dot(N, L), 0.0, 1.0);

	vec3 colour = vec3(0.0);

	if (NdotL > 0.0) {
		
		float D = GGX_Distribution(NdotH, roughness); 
		float G = GeometryShlickGGX(NdotV, NdotL, roughness);
		vec3 F = FresnelSchlick(NdotV, F0);		
		
		vec3 specularContribution = D * F * G / (4.0 * NdotL * NdotV + 0.001);		
		vec3 diffuseContribution = (vec3(1.0) - F) * (1.0 - metallic);
		diffuseContribution *= albedo / PI;
		colour = (diffuseContribution + specularContribution) * radiance * NdotL; 
	}

	return colour;
}

#endif // PBR_H