#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform CameraUbo 
{
	mat4 mvp;
	mat4 projection;
	mat4 view;
	mat4 model;
} camera_ubo;

layout (location = 0) out vec3 outUv;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	
	gl_Position = camera_ubo.projection * camera_ubo.model * vec4(inPos.xyz, 1.0);

	outUv = inPos;
}