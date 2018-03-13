#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec3 inColour;
layout(location = 3) in vec3 inViewMatrix;
layout(location = 4) in vec3 inLightPos;

layout(location = 0) out vec4 fragColour;

layout(set = 1, binding = 0) uniform sampler2D diffuseMap;
layout(set = 1, binding = 1) uniform sampler2D specularMap;

layout(push_constant) uniform Material
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float opacity;
} mat;

void main()
{
	vec4 diffColour = texture(diffuseMap, inUv) * vec4(inColour, 1.0);
	vec4 specColour = texture(specularMap, inUv);
	
	// ambient
	vec3 ambient = mat.ambient.rgb * diffColour.rgb;
	
	//diffuse
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightPos);
	vec3 diff = max(dot(N, L), 0.0) * mat.diffuse.rgb;
	vec3 diffuse = diff * diffColour.rgb;
	
	//specular
	vec3 V = normalize(inViewMatrix);
	vec3 R = reflect(-L, N);
	vec3 spec = pow(max(dot(R, V), 0.0), 16.0) * mat.specular.rgb;		
	vec3 specular = spec * specColour.rgb;
	
	fragColour = vec4(ambient + diffuse + specular, 1.0 - mat.opacity);
}
	
	