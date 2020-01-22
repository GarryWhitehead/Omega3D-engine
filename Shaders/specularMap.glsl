////////////////////////////////////////
##pipeline:

// depth-stencil
DepthTestEnable=True;
DepthWriteEnable=True; 
CompareOp=LessOrEqual;

// raster
PolygonMode=Fill;
CullMode=Back;
FrontFace=CounterClockwise;

// sampler
MagFilter=Nearest;
MinFilter=Nearest;
AddressModeU=ClampToEdge;
AddressModeV=ClampToEdge;
AddressModeW=ClampToEdge;

##end_pipeline

/////////////////////////////////////
##stage: Vertex

#input: Name=Pos, Type=vec3;
#output: Name=Uv, Type=vec3;

#push_constant: Name=PushBuffer, id=push;
#item: Name=mvp, Type=mat4, Offset=0;

#code_block:
void main()
{
	outUv = inPos;
	
	gl_Position = push.mvp * vec4(inPos.xyz, 1.0);
}
#end_code_block

##end_stage

////////////////////////////////////
##stage: Fragment

#import_sampler: Name=envSampler, Type=Cube_Sampler;
#output: Name=Col, Type=vec4;

#push_constant: Name=PushBuffer, id=push;
#item: Name=roughness, Type=float, Offset=64;
#item: Name=sampleCount, Type=int, Offset=68;

#code_block:
#define PI 3.1415926535897932384626433832795

vec2 Hammersley(uint i, uint N)
{
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rad = float(bits) * 2.3283064365386963e-10; 							// 0x100000000
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


float GGX_Distribution(float NdotH, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	
	return (a2)/(PI * denom * denom); 
}

void main()
{
	vec3 N = normalize(inUv);
	vec3 V = N;
	
	float totalWeight = 0.0;
	vec3 preFilterCol = vec3(0.0);

	for(uint c = 0; c < push.sampleCount; c++) {
	
		vec2 Xi = Hammersley(c, push.sampleCount);
		vec3 H = GGX_ImportanceSample(Xi, N, push.roughness);
		
		vec3 L = 2.0 * dot(V, H) * H - V;
		
		float NdotL = clamp(dot(N, L), 0.0, 1.0);
		float NdotH = clamp(dot(N, H), 0.0, 1.0);
		float HdotV = clamp(dot(H, V), 0.0, 1.0);
		
		if(NdotL > 0.0) {
		
			float D = GGX_Distribution(NdotH, push.roughness);
			float pdf = (D * NdotH / (4.0 * HdotV)) + 0.0001;
			
			float resolution = float(textureSize(envSampler, 0).s);
			float saTex = 4.0 * PI / (6.0 * resolution * resolution);
			float saSample = 1.0 / (float(push.sampleCount) * pdf + 0.0001);
			
			float mipLevel = push.roughness == 0.0 ? 0.0 : max(0.5 * log2(saSample / saTex) + 1.0, 0.0);
			
			preFilterCol += textureLod(envSampler, L, mipLevel).rgb * NdotL;
			totalWeight += NdotL;
		}
	}
	
	outCol = vec4(preFilterCol / totalWeight, 1.0);
}
#end_code_block

##end_stage
