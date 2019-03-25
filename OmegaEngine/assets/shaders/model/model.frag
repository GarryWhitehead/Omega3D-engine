#version 450

layout (set = 0, binding = 0) uniform sampler2D baseColourMap;
layout (set = 0, binding = 1) uniform sampler2D normalMap;
layout (set = 0, binding = 2) uniform sampler2D mrMap;
layout (set = 0, binding = 3) uniform sampler2D emissiveMap;
layout (set = 0, binding = 4) uniform sampler2D aoMap;

layout (location = 0) in vec2 inUv;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inPos;

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

#define EPSILON 0.00001

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
	// convert normal to -1, 1 coord system
	vec3 tangentNormal = texture(normalMap, inUv).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inPos);	// edge1
	vec3 q2 = dFdy(inPos);	// edge2
	vec2 st1 = dFdx(inUv);	// uv1
	vec2 st2 = dFdy(inUv);	// uv2

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
		if (material.haveBaseColourMap > 0) {
			baseColour = texture(baseColourMap, inUv) * material.baseColorFactor;
		}
		else {
			baseColour = material.baseColorFactor;
		}
		if (baseColour.a < material.alphaMaskCutoff) {
			discard;
		}

		//baseColour.xyz *= inColour;		// vertex colour
	}

	// normal
	vec3 normal; 
	if (material.haveNormalMap > 0) {

		normal = peturbNormal();
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
			vec4 mrSample = texture(mrMap, inUv);
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
		if (material.haveMrMap > 0) {
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
	if (material.haveAoMap > 0) {
        ambient = texture(aoMap, inUv).x;
	}
	outColour.a = ambient;

	// emmisive
	vec3 emissive;
	if (material.haveEmissiveMap > 0) {
        emissive = texture(emissiveMap, inUv).rgb;
		emissive *= material.emissiveFactor.rgb;
	}
	else { 
   		emissive = material.emissiveFactor.rgb;
	}
	outEmissive = vec4(emissive, 1.0);
	
	outPosition = vec4(inPos, 1.0);
}