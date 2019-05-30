#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform CameraUbo 
{
	mat4 mvp;
	mat4 projection;
	mat4 view;
} camera_ubo;

layout (location = 0) out vec3 outUv;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	mat4 viewMatrix = mat4(mat3(camera_ubo.view));
	gl_Position = camera_ubo.projection * viewMatrix * vec4(inPos.xyz, 1.0);
	
	// ensure skybox is renderered on the far plane
	gl_Position.z = gl_Position.w;		
	gl_Position.y *= -1.0;

	outUv = inPos;
}