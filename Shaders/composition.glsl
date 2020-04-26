////////////////////////////////////////
##pipeline:

// depth-stencil
DepthTestEnable=True;
DepthWriteEnable=False;
CompareOp=LessOrEqual;

// raster
PolygonMode=Fill;
CullMode=Back;
FrontFace=Clockwise;

// sampler
MagFilter=Nearest;
MinFilter=Nearest;
AddressModeU=ClampToEdge;
AddressModeV=ClampToEdge;
AddressModeW=ClampToEdge;

##end_pipeline

/////////////////////////////////////
##stage: Vertex

#output: Name=Uv, 		 Type=vec2;
	
#code_block:
void main()
{	
	outUv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUv * 2.0f - 1.0f, 0.0f, 1.0f);
}

#end_code_block

##end_stage

////////////////////////////////////
##stage: Fragment

#import_sampler: Name=compositionRT, Type=2D_Sampler;

#output: Name=Frag, Type=vec4;

#push_constant: Name=ToneMapPush, id=push;
[[
	Name=expBias, Type=float;
	Name=gamma,   Type=float;
]]

#code_block:

vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main()
{
	vec3 finalColour = texture(compositionRT, inUv).rgb;
	
	// tone mapping - from http://filmicworlds.com/blog/filmic-tonemapping-operators/
	finalColour = Uncharted2Tonemap(push.expBias * finalColour);
	
	vec3 whiteScale = vec3(1.0 / Uncharted2Tonemap(vec3(11.2)));
	finalColour *= whiteScale;
	finalColour = pow(finalColour, vec3(1.0 / push.gamma));		// to the power of 1/gamma
	
	outFrag = vec4(finalColour, 1.0);
}

#end_code_block

##end_stage
