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
#output: Name=CameraPos, Type=vec3;

#import_buffer: Name=CameraUbo, Type=UniformBuffer, id=ubo;
[[
	Name=mvp, 			Type=mat4;
	Name=projection,	Type=mat4;
	Name=view, 			Type=mat4;
    Name=model,         Type=mat4;
	Name=cameraPos,		Type=vec3;
	Name=pad0, 			Type=float;
]]
	
#code_block:
void main()
{	
	outUv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(outUv * 2.0f - 1.0f, 0.0f, 1.0f);

	outCameraPos = ubo.cameraPos;
}
#end_code_block

##end_stage

////////////////////////////////////
##stage: Fragment

#include_file: "include/pbr.h"
#include_file: "include/lights.h"
#include_file: "include/shadow.h"

#constant: Name=MAX_SPOT_LIGHTS, 	Type=int, 	Value=20;
#constant: Name=MAX_POINT_LIGHTS,	Type=int, 	Value=20;
#constant: Name=MAX_DIR_LIGHTS, 	Type=int, 	Value=2;

#push_constant: Name=PushBuffer,	id=push;
[[
  Name=IBLAmbient,  Type=float;
]]

#import_sampler: Name=positionRT, 			Type=2D_Sampler;
#import_sampler: Name=baseColourRT,			Type=2D_Sampler;
#import_sampler: Name=normalRT,				Type=2D_Sampler;
#import_sampler: Name=pbrRT,				Type=2D_Sampler;
#import_sampler: Name=emissiveRT,			Type=2D_Sampler;
//#import_sampler: Name=shadowSampler,		Type=2D_Sampler, 	Variant=SHADOWS_ENABLED;
#import_sampler: Name=bdrfSampler,			Type=2D_Sampler,	Variant=IBL_ENABLED;
#import_sampler: Name=irradianceSampler,	Type=Cube_Sampler,	Variant=IBL_ENABLED;
#import_sampler: Name=specularSampler,		Type=Cube_Sampler,	Variant=IBL_ENABLED;

#import_buffer: Name=LightUbo, Type=UniformBuffer,	id=light_ubo;
[[
  Name=spotLights, 	Type={External}SpotLight, 		    Array_size={constant}MAX_SPOT_LIGHTS;
  Name=pointLights, Type={External}PointLight, 		    Array_size={constant}MAX_POINT_LIGHTS;
  Name=dirLights, 	Type={External}DirectionalLight, 	Array_size={constant}MAX_DIR_LIGHTS;
]]

#output: Name=Frag, Type=vec4;

#code_block:

#ifdef IBL_ENABLED
vec3 calculateIBL(vec3 N, float NdotV, float roughness, vec3 reflection, vec3 diffuseColour, vec3 specularColour)
{	
	vec3 bdrf = (texture(bdrfSampler, vec2(NdotV, 1.0 - roughness))).rgb;
	
	// specular contribution
	// this should be a pbr input!
	const float maxLod = 5.0;
	
	float lod = maxLod * roughness;
	float lodf = floor(lod);
	float lodc = ceil(lod);
	
	vec3 a = textureLod(specularSampler, reflection, lodf).rgb;
	vec3 b = textureLod(specularSampler, reflection, lodc).rgb;
	vec3 specularLight = mix(a, b, lod - lodf);
	
	vec3 specular = specularLight * (specularColour * bdrf.x + bdrf.y);
	
	// diffuse contribution
	vec3 diffuseLight = texture(irradianceSampler, N).rgb;
	vec3 diffuse = diffuseLight * diffuseColour;
	
	diffuse *= push.IBLAmbient;
	specular *= push.IBLAmbient;
	
	return diffuse + specular;
}
#endif

void main()
{	
	vec3 inPos = texture(positionRT, inUv).rgb;
	vec3 V = normalize(inCameraPos.xyz - inPos);
	vec3 N = texture(normalRT, inUv).rgb;
	vec3 R = normalize(-reflect(V, N));
	
	// get colour information from G-buffer
	vec3 baseColour = texture(baseColourRT, inUv).rgb;
	float metallic = texture(pbrRT, inUv).x;
	float roughness = texture(pbrRT, inUv).y;
	float occlusion = texture(baseColourRT, inUv).a;
	vec3 emissive = texture(emissiveRT, inUv).rgb;
	
	vec3 F0 = vec3(0.04);
	vec3 specularColour = mix(F0, baseColour, metallic);
	
	float reflectance = max(max(specularColour.r, specularColour.g), specularColour.b);
	float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);  // 25.0-50.0 is used
	vec3 specReflectance = specularColour.rgb;
	vec3 specReflectance90 = vec3(1.0, 1.0, 1.0) * reflectance90;
	
	float alphaRoughness = roughness * roughness;
	
	// apply additional lighting contribution to specular 
	vec3 colour = vec3(0.0);
		
	// spot lights - count baked in at the moment - REMEMBER to change!
	for(int i = 0; i < 1; ++i) 
	{  
		SpotLight light = light_ubo.spotLights[i];
		
		vec3 lightPos = light.pos.xyz - inPos;
		vec3 L = normalize(lightPos);
		float intensity = light.colour.a;
	
		float attenuation = calculateDistance(lightPos, light.fallOut);
		attenuation *= calculateAngle(light.direction.xyz, L, light.scale, light.offset); 	
		
		colour += specularContribution(L, V, N, baseColour, metallic, alphaRoughness, attenuation, intensity, light.colour.rgb, specReflectance, specReflectance90);
	}
	
	// point lights
	for(int i = 0; i < 1; ++i) 
	{  
		PointLight light = light_ubo.pointLights[i];
		
		vec3 lightPos = light.pos.xyz - inPos;
		vec3 L = normalize(lightPos);
		float intensity = light.colour.a;
		
		float attenuation = calculateDistance(lightPos, light.fallOut);
		colour += specularContribution(L, V, N, baseColour, metallic, alphaRoughness, attenuation, intensity, light.colour.rgb, specReflectance, specReflectance90);
	}
	
	// directional lights
	for(int i = 0; i < 1; ++i) 
	{  
		DirectionalLight light = light_ubo.dirLights[i];
		
		//vec3 L = light.direction.xyz;
		vec3 L = calculateSunArea(light.direction.xyz, light.pos.xyz, R);
		float intensity = light.colour.a;
		float attenuation = 1.0f;
		colour += specularContribution(L, V, N, baseColour, metallic, alphaRoughness, attenuation, intensity, light.colour.rgb, specReflectance, specReflectance90);
	}
	
	// add IBL contribution if needed
#ifdef IBL_ENABLED
	float NdotV = max(dot(N, V), 0.0);
	colour += calculateIBL(N, NdotV, roughness, R, baseColour, specularColour);
#endif
	
	// occlusion
	colour = mix(colour, colour * occlusion, 1.0);
	
	// emissive 
	colour += emissive; 
		
	outFrag = vec4(colour, 1.0);
	
#ifdef SHADOWS_ENABLED
	// finally adjust the colour if in shadow for each light source
	for(int i = 0; i < 2; i++) 
	{
		SpotLight light = light_ubo.spotLights[i];
		
		vec4 shadowClip	= light.viewMatrix * vec4(inPos, 1.0);
		float shadowFactor = shadowPCF(shadowClip, shadowSampler);
			
		outFrag *= shadowFactor;
	}
	
	for(int i = 0; i < 1; i++) 
	{
		PointLight light = light_ubo.pointLights[i];
		
		vec4 shadowClip	= light.viewMatrix * vec4(inPos, 1.0);
		float shadowFactor = shadowPCF(shadowClip, shadowSampler);
			
		outFrag *= shadowFactor;
	}
#endif
} 
#end_code_block

##end_stage
