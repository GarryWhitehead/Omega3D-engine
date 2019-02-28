#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform UBOBuffer
{
	mat4 projection;
	mat4 modelMatrix;
	mat4 viewMatrix;
	vec4 cameraPos;
	vec4 perlinOctave;
	vec4 perlinAmplitude;
	vec2 perlinMovement;
	vec2 dim;
	float dispFactor;
	float tessFactor;
	float tessEdgeSize;
} ubo;

layout (set = 1, binding = 1) uniform sampler2D disSampler;

layout (vertices = 4) out;

layout (location = 0) in vec2 inUv[];
layout (location = 1) in vec3 inPos[];

layout (location = 0) out vec2 outUv[4]; 	// four vertices
layout (location = 1) out vec3 outPos[4];

vec4 WorldToNormalSpace(vec4 vert)
{
	vec4 norm = ubo.projection * ubo.viewMatrix * ubo.modelMatrix * vert;
	norm / norm.w;
	return norm;
}

float ConvertToTessFactor(vec4 vert0, vec4 vert1)
{
	vec4 mid = (vert0 + vert1) / 2.0;
	float dist = distance(vert0, vert1) / 2.0;
	
	vec4 view = ubo.viewMatrix * ubo.modelMatrix * mid;
	
	vec4 c0 = (ubo.projection * (view - vec4(dist, vec3(0.0))));
	vec4 c1 = (ubo.projection * (view + vec4(dist, vec3(0.0))));
	
	c0 /= c0.w;
	c1 /= c1.w;
	
	c0.xy *= ubo.dim;
	c1.xy *= ubo.dim;
	
	return clamp(distance(c0, c1) / ubo.tessEdgeSize * ubo.tessFactor, 1.0, 64.0);
}

bool OffscreenCheck(vec4 vertex)
{
	vec4 vert = WorldToNormalSpace(vertex);
	
	if(vert.z < -0.5) {
		return true;
	}
	if(vert.x < -1.7 || vert.x > 1.7) {
		return true;
	}
	else if (vert.y < -1.7 || vert.y > 1.7) {
		return true;
	}
	return false;
}

bool FustrumCheck()
{	
	if(OffscreenCheck(gl_in[0].gl_Position) && OffscreenCheck(gl_in[1].gl_Position) &&
		OffscreenCheck(gl_in[2].gl_Position) && OffscreenCheck(gl_in[3].gl_Position)) {
			return false;
		}
	
	return true;
}
	
void main()
{
	if(gl_InvocationID == 0) {
		
		//if(!FustrumCheck()) {
		
		//	gl_TessLevelInner[0] = 0;
		//	gl_TessLevelInner[1] = 0;
		//	gl_TessLevelOuter[0] = 0;
		//	gl_TessLevelOuter[1] = 0;
		//	gl_TessLevelOuter[2] = 0;
		//	gl_TessLevelOuter[3] = 0;
		//}
		//else {
		
			if(ubo.tessFactor > 0.0) {
		
				gl_TessLevelOuter[0] = ConvertToTessFactor(gl_in[3].gl_Position, gl_in[0].gl_Position);
				gl_TessLevelOuter[1] = ConvertToTessFactor(gl_in[0].gl_Position, gl_in[1].gl_Position);
				gl_TessLevelOuter[2] = ConvertToTessFactor(gl_in[1].gl_Position, gl_in[2].gl_Position);
				gl_TessLevelOuter[3] = ConvertToTessFactor(gl_in[2].gl_Position, gl_in[3].gl_Position);
				gl_TessLevelInner[0] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[3], 0.5);
				gl_TessLevelInner[1] = mix(gl_TessLevelOuter[2], gl_TessLevelOuter[1], 0.5);
			}
			else {
			
				gl_TessLevelInner[0] = 1.0;
				gl_TessLevelInner[1] = 1.0;
				gl_TessLevelOuter[0] = 1.0;
				gl_TessLevelOuter[1] = 1.0;
				gl_TessLevelOuter[2] = 1.0;
				gl_TessLevelOuter[3] = 1.0;
			}
		//}
	}
	
	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
	outUv[gl_InvocationID] = inUv[gl_InvocationID];
}