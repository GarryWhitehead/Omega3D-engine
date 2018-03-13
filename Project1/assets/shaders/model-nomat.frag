#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUv;				// not used in the no-material version
layout(location = 2) in vec3 inColour;
layout(location = 3) in vec3 inViewMatrix;
layout(location = 4) in vec3 inLightPos;

layout(location = 0) out vec4 fragColour;

void main()
{	
	// ambient
	vec3 ambient = vec3(0.5);
	
	//diffuse
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightPos);
	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);
		
	fragColour = vec4((ambient + diffuse) * inColour, 1.0);  
}
	
	