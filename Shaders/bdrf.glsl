#pragma vertex_shader_begin

void main()
{
	outUv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUv * 2.0f - 1.0f, 0.0f, 1.0f);
}

#pragma fragment_shader_begin

vec2 Hammersley(uint i, uint N)
{
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	float rad = float(bits) * 2.3283064365386963e-10; 		// 0x100000000
	return vec2(float(i) / float(N), rad);
}

vec3 GGX_ImportanceSample(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness * roughness;
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	
	// spherical to cartesian coordinates
	vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
 
	// from tangent-space vector to world-space sample vector
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tanX = normalize(cross(up, N));
	vec3 tanY = normalize(cross(N, tanX));
	return normalize(tanX * H.x + tanY * H.y + N * H.z);
}  

float GeometryShlickGGX(float NdotV, float NdotL, float roughness)
{
	float k = (roughness * roughness) / 2.0;
	float GV = NdotV / (NdotV * (1.0 - k) + k);
	float GL = NdotL / (NdotL * (1.0 - k) + k);
	return GL * GV;
}

vec2 IntegrateBRDF(float NdotV, float roughness)
{
	float NoV = inUv.s;
	
	vec3 N = vec3(0.0, 0.0, 1.0);
	vec3 V = vec3(sqrt(1.0 - NoV * NoV), 0.0, NoV);
	
	vec2 lut = vec2(0.0);
	for(int c = 0; c < sampleCount; c++) {

		vec2 Xi = Hammersley(c, sampleCount);
		vec3 H = GGX_ImportanceSample(Xi, N, roughness);
		
		vec3 L = normalize(2.0 * dot(V, H) * H - V);
		
		float NdotL = max(dot(N, L), 0.0);
		float NdotH = max(dot(N, H), 0.0);
		float NdotV = max(dot(N, V), 0.0);
		float HdotV = max(dot(H, V), 0.0);
	
		if(NdotL > 0.0) {
		
			// cook-torrance BDRF calculations
			float G = GeometryShlickGGX(NdotV, NdotL, roughness);
			float G_vis = (G * HdotV) / (NdotH * NdotV);
			float Fc = pow(1.0 - HdotV, 5.0);
			lut += vec2((1.0 - Fc) * G_vis, Fc * G_vis);
		}
	}
	return lut / sampleCount;
}

void main()
{
	outCol = vec4(IntegrateBRDF(inUv.s, 1.0 - inUv.t), 0.0, 1.0);
}