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
[[
  Name=mvp, Type=mat4;
]]

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

#code_block:
#define PI 3.1415926535897932384626433832795

void main()
{
	vec3 N = normalize(inUv);	// i.e. the position
	vec3 up = vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, N));
	up = cross(N, right);
	
	vec3 irrColour = vec3(0.0);
	float sampleCount = 0.0;
		
	float doublePI = PI * 2;
	float halfPI = PI * 0.5;

	float dPhi = 0.035;
	float dTheta = 0.025;
	
	for(float phi = 0.0; phi < doublePI; phi += dPhi) {
	
		for(float theta = 0.0; theta < halfPI; theta += dTheta) {
			
			vec3 tempVec = cos(phi) * right + sin(phi) * up;
			vec3 sampleVector = cos(theta) * N + sin(theta) * tempVec;
			irrColour += texture(envSampler, sampleVector).rgb * cos(theta) * sin(theta);
			
			// spherical to cartesian
			//vec3 tanSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
			
			// to world space
			//vec3 wPos = tanSample.x * right + tanSample.y * up + tanSample.z * N;
			
			//irrColour += texture(envSampler, wPos).rgb * cos(theta) * sin(theta);
			sampleCount++;
		}
	}
	
	outCol = vec4(PI * irrColour / float(sampleCount), 1.0);
}
#end_code_block

##end_stage
