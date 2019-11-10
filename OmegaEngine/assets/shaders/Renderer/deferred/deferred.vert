#version 450

layout (set = 0, binding = 0) uniform CameraUbo
{
	mat4 mvp;
	mat4 projection;
	mat4 view;
	mat4 model;
	vec3 cameraPos;
	float pad0;
} ubo;

layout (location = 0) out vec2 outUv;
layout (location = 1) out vec3 outCameraPos;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main()
{
	outUv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUv * 2.0f - 1.0f, 0.0f, 1.0f);

	outCameraPos = ubo.cameraPos;
}