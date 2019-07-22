#ifndef PBR_H
#define PBR_H

#define PI 3.1415926535897932384626433832795

// microfacet distribution across the surface
float GGX_Distribution(float NdotH, float roughness)
{
	float r2 = roughness * roughness;
	float denom = (NdotH * r2 - NdotH) * NdotH + 1.0;
	return r2 / (PI * denom * denom); 
}

// specular geometric attenuation - rougher surfaces reflect less light
float GeometryShlickGGX(float NdotV, float NdotL, float roughness)
{
	float r2 = roughness * roughness;
	float GL = 2.0 * NdotL / (NdotL + sqrt(r2 + (1.0 - r2) * (NdotL * NdotL)));
	float GV = 2.0 * NdotV / (NdotV + sqrt(r2 + (1.0 - r2) * (NdotV * NdotV)));
	return GL * GV;
}

// Fresnal reflectance part part of the specular equation
vec3 FresnelSchlick(float VdotH, vec3 specReflectance, vec3 specReflectance90)
{
	return specReflectance + vec3(specReflectance90 - specReflectance) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

vec3 FresnelRoughness(float VdotH, vec3 F0, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - VdotH, 5.0);
}

vec3 specularContribution(vec3 L, vec3 V, vec3 N, float metallic, float roughness, vec3 albedo, vec3 radiance, vec3 specReflectance, vec3 specReflectance90)
{
	vec3 H = normalize(V + L);
	float NdotH = clamp(dot(N, H), 0.0, 1.0);
	float NdotV = clamp(dot(N, V), 0.001, 1.0);
	float NdotL = clamp(dot(N, L), 0.001, 1.0);
	float VdotH = clamp(dot(V, H), 0.0, 1.0);

	vec3 colour = vec3(0.0);

	if (NdotL > 0.0) {
		
		float D = GGX_Distribution(NdotH, roughness); 
		float G = GeometryShlickGGX(NdotV, NdotL, roughness);
		vec3 F = FresnelSchlick(VdotH, specReflectance, specReflectance90);		
		
		vec3 specularContribution = F * G * D / (4.0 * NdotL * NdotV);		
		vec3 diffuseContribution = (vec3(1.0) - F) * (albedo / PI);
		colour = (diffuseContribution + specularContribution) * radiance * NdotL; 
	}

	return colour;
}

#endif // PBR_H