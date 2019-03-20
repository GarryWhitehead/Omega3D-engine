#version 450

layout (set = 0, binding = 0) uniform sampler2D baseColourMap;
layout (set = 0, binding = 1) uniform sampler2D normalMap;
layout (set = 0, binding = 2) uniform sampler2D mrMap;
layout (set = 0, binding = 3) uniform sampler2D emissiveMap;
layout (set = 0, binding = 4) uniform sampler2D aoMap;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColour;
layout (location = 3) in vec3 inPos;

layout(push_constant) uniform pushConstants 
{
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	vec4 diffuseFactor;
	vec4 specularFactor;
	float metallicFactor;	
	float roughnessFactor;	
	float alphaMask;	
	float alphaMaskCutoff;
	bool haveBaseColourMap;
	bool haveNormalMap;
	bool haveEmissiveMap;
	bool haveMrMap;
	bool haveAoMap;
	int workflow;

} material;

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec4 outColour;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec2 outPbr;
layout (location = 4) out vec3 outEmissive;

#define EPSILON 0.00001

#define PBR_WORKFLOW_METALLIC_ROUGHNESS 0
#define PBR_WORKFLOW_SPECULAR_GLOSINESS 1

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
vec3 peturbNormal()
{
	
	vec3 tangentNormal = texture(normalMap, inUv).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inPos);
	vec3 q2 = dFdy(inPos);
	vec2 st1 = dFdx(inUv);
	vec2 st2 = dFdy(inUv);

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

	if (material.alphaMask == 1.0) {
		if (material.haveBaseColourMap) {
			baseColour = texture(baseColourMap, inUv) * material.baseColorFactor;
		}
		else {
			baseColour = material.baseColorFactor;
		}
		if (baseColour.a < material.alphaMaskCutoff) {
			discard;
		}

		baseColour.xyz *= inColour;		// vertex colour
	}

	// normal
	vec3 normal; 
	if (material.haveNormalMap) {

		onormal = peturbNormal();
	}
	else {
		normal = normalize(inNormal);
	}
	outNormal = normal;

	// diffuse - based on metallic and roughness
	float roughness = 0.0;
	float metallic = 0.0;
	if (material.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {

		roughness = material.roughnessFactor;
		metallic = material.metallicFactor;

		if (material.haveMrMap) {
			vec4 mrSample = texture(mrMap, inUv);
			roughness = mrSample.g * roughness;
			metallic = mrSample.b * metallic;
		} 
		else {
			roughness = clamp(roughness, 0.04, 1.0);
			metallic = clamp(metallic, 0.0, 1.0);
		}
	}

	if (material.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS) {
		// Values from specular glossiness workflow are converted to metallic roughness
		if (material.haveMrMap) {
			roughness = 1.0 - texture(mrMap, inUv).a;
		} else {
			roughness = 0.0;
		}

		vec4 diffuse = texture(baseColourMap, inUv);
		vec3 specular = texture(mrMap, inUv).rgb;

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
	if (material.haveAoMap) {
        ambient = texture(aoMap, inUv).x;
	}
	outColour.a = ambient;

	// emmisive
	vec3 emissive;
	if (material.haveEmissiveMap) {
        emissive = texture(emissiveMap, inUv).rgb;
		emissive *= material.emissiveFactor.rgb;
	}
	else { 
   		emissive = material.emissiveFactor.rgb;
	}
	outEmissive = emissive;
	
	outPosition = inPos;
}