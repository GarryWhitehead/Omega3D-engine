#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform samplerCube envSampler; 

layout (location = 0) out vec4 outPos;		// will be used for calculations in the deferred step

#define PI 3.1415926535897932384626433832795

void main()
{
	vec3 N = normalize(inPos);
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = cross(N, right);
	
	vec3 irrColour = vec3(0.0);
	float sampleCount = 0.0;
	
	float dPhi = 0.025;
	float dTheta = 0.025;
	
	float doublePI = PI * 2;
	float halfPI = PI * 0.5;
	
	for(float phi = 0.0; phi < doublePI; phi += dPhi) {
	
		for(float theta = 0.0; theta < halfPI; theta += dTheta) {
		
			// spherical to cartesian
			vec3 tanSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			
			// to world space
			vec3 wPos = tanSample.x * right + tanSample.y * up + tanSample.z * N;
			
			irrColour += texture(envSampler, wPos).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}
	
	outPos = vec4(PI * irrColour / float(sampleCount), 1.0);
}