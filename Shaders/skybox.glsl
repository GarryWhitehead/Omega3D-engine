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

#import_buffer: Name=CameraUbo, Type=UniformBuffer, id=camera_ubo;
[[
	Name=mvp, 			Type=mat4;
	Name=projection,	Type=mat4;
	Name=view, 			Type=mat4;
]]

#code_block:

void main() 
{
	mat4 viewMatrix = mat4(mat3(camera_ubo.view));
	gl_Position = camera_ubo.projection * viewMatrix * vec4(inPos.xyz, 1.0);
	
	// ensure skybox is renderered on the far plane
	gl_Position.z = gl_Position.w;		

	outUv = inPos;
}
#end_code_block

##end_stage

////////////////////////////////////
##stage: Fragment

#import_sampler: Name=envSampler, Type=Cube_Sampler;
#output: Name=Col, Type=vec4;

#push_constant: Name=PushBuffer, id=push;
[[
  Name=blurFactor, Type=float;
]]

#code_block:

void main() 
{	
	outCol = textureLod(envSampler, inUv, push.blurFactor);
}
#end_code_block

##end_stage
