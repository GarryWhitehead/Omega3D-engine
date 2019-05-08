#version 450

layout (set = 2, binding = 0) uniform sampler2D baseColourMap;
layout (set = 2, binding = 1) uniform sampler2D normalMap;
layout (set = 2, binding = 2) uniform sampler2D mrMap;
layout (set = 2, binding = 3) uniform sampler2D emissiveMap;
layout (set = 2, binding = 4) uniform sampler2D aoMap;

layout (location = 0) in vec2 inUv0;
layout (location = 1) in vec2 inUv1;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inPos;

layout(push_constant) uniform pushConstants 
{
	vec4 baseColorFactor;
	vec3 emissiveFactor;
	float pad0;
	vec3 diffuseFactor;
	float pad1;
	vec3 specularFactor;
	float pad2;
	float metallicFactor;	
	float roughnessFactor;	
	float alphaMask;	
	float alphaMaskCutoff;
	uint baseColourUvSet;
	uint metallicRoughnessUvSet;
	uint normalUvSet;
	uint emissiveUvSet;
	uint occlusionUvSet;
	uint haveBaseColourMap;
	uint haveNormalMap;
	uint haveEmissiveMap;
	uint haveMrMap;
	uint haveAoMap;
	uint usingSpecularGlossiness;

} material;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outColour;
layout (location = 2) out vec4 outNormal;
layout (location = 3) out vec2 outPbr;
layout (location = 4) out vec4 outEmissive;

#define EPSILON 0.0000001

float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular)
{
	float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
	float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
	
	if (perceivedSpecular < 0.04) {
		return 0.0;
	}
	
	float a = 0.04;
	float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - 0.04) + perceivedSpecular - 2.0 * 0.04;
	float c = 0.04 - perceivedSpecular;
	float D = max(b * b - 4.0 * a * c, 0.0);
	
	return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

// The most copied function in the world! From here: http://www.thetenthplanet.de/archives/1180
vec3 peturbNormal(vec2 tex_coord)
{
	// convert normal to -1, 1 coord system
	vec3 tangentNormal = texture(normalMap, tex_coord).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inPos);			// edge1
	vec3 q2 = dFdy(inPos);			// edge2
	vec2 st1 = dFdx(tex_coord);		// uv1
	vec2 st2 = dFdy(tex_coord);		// uv2

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main()
{
	// albedo
	vec4 baseColour;
	
	// uv sets for all textures
	vec2 baseColour_uv = material.baseColourUvSet == 0 ? inUv0 : inUv1;
	vec2 normal_uv = material.normalUvSet == 0 ? inUv0 : inUv1;
	vec2 mr_uv = material.metallicRoughnessUvSet == 0 ? inUv0 : inUv1;
	vec2 emissive_uv = material.emissiveUvSet == 0 ? inUv0 : inUv1;
	vec2 occlusion_uv = material.occlusionUvSet == 0 ? inUv0 : inUv1;
	
	if (material.alphaMask == 0.0) {
		if (material.haveBaseColourMap > 0) {
			baseColour = texture(baseColourMap, baseColour_uv) * material.baseColorFactor;
		}
		else {
			baseColour = material.baseColorFactor;
		}
		if (baseColour.a < material.alphaMaskCutoff) {
			discard;
		}	
	}

	// normal
	vec3 normal; 
	if (material.haveNormalMap > 0) {

		normal = peturbNormal(normal_uv);
	}
	else {
		normal = normalize(inNormal);
	}
	outNormal = vec4(normal, 1.0);

	// diffuse - based on metallic and roughness
	float roughness = 0.0;
	float metallic = 0.0;
	if (material.usingSpecularGlossiness == 0) {

		roughness = material.roughnessFactor;
		metallic = material.metallicFactor;

		if (material.haveMrMap > 0) {
			vec4 mrSample = texture(mrMap, mr_uv);
			roughness = mrSample.g * roughness;
			metallic = mrSample.b * metallic;
		} 
		else {
			roughness = clamp(roughness, 0.04, 1.0);
			metallic = clamp(metallic, 0.0, 1.0);
		}
	}

	else {
		// Values from specular glossiness workflow are converted to metallic roughness

		vec4 diffuse;
		vec3 specular;

		if (material.haveMrMap > 0) {
			roughness = 1.0 - texture(mrMap, mr_uv).a;
			specular = texture(mrMap, mr_uv).rgb;

		} else {
			roughness = 0.0;
			specular = vec3(0.0);
		}
		
		if (material.haveBaseColourMap > 0) {
			diffuse = texture(baseColourMap, baseColour_uv);
		}
		else {
			diffuse = material.baseColorFactor;
		}

		float maxSpecular = max(max(specular.r, specular.g), specular.b);

		// Convert metallic value from specular glossiness inputs
		metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);

		vec3 baseColourDiffusePart = diffuse.rgb * ((1.0 - maxSpecular) / (1 - 0.04) / max(1 - metallic, EPSILON)) * material.diffuseFactor.rgb;
		vec3 baseColourSpecularPart = specular - (vec3(0.04) * (1 - metallic) * (1 / max(metallic, EPSILON))) * material.specularFactor.rgb;
		baseColour = vec4(mix(baseColourDiffusePart, baseColourSpecularPart, metallic * metallic), diffuse.a);

	}
	outColour = baseColour;
	outPbr = vec2(metallic, roughness);

	// ao
	float ambient = 1.0;
	if (material.haveAoMap > 0) {
        ambient = texture(aoMap, occlusion_uv).x;
	}
	outColour.a = ambient;

	// emmisive
	vec3 emissive;
	if (material.haveEmissiveMap > 0) {
        emissive = texture(emissiveMap, emissive_uv).rgb;
		emissive *= material.emissiveFactor.rgb;
	}
	else { 
   		emissive = material.emissiveFactor.rgb;
	}
	outEmissive = vec4(emissive, 1.0);
	
	outPosition = vec4(inPos, 1.0);
}