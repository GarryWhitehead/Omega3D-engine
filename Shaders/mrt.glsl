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

#constant: Name=MAX_BONES, Type=int, Value=250;

#input: Name=Pos, 		Type=vec4;
#input: Name=Uv, 		Type=vec2;
#input: Name=Normal, 	Type=vec3;
#input: Name=Tangent,	Type=vec4,	Variant=HAS_TANGENT;
#input: Name=BiTangent,	Type=vec4,	Variant=HAS_BITANGENT;
#input: Name=Weights, 	Type=vec4,	Variant=HAS_WEIGHTS;
#input: Name=BoneId, 	Type=vec4,	Variant=HAS_BONES;

#output: Name=Uv, 		Type=vec2;
#output: Name=Normal, 	Type=vec3;
#output: Name=Pos, 		Type=vec3;

#import_buffer: Name=CameraUbo, Type=UniformBuffer, id=camera_ubo;
[[
	Name=mvp, Type=mat4;
]]

#import_buffer:	Name=MeshUbo, Type=DynamicUniform, id=mesh_ubo;
[[
	Name=modelMatrix, Type=mat4;
]]

#import_buffer:	Name=SkinUbo, Type=DynamicUniform, id=skin_ubo, Variant=HAS_SKIN;
[[
	Name=bones, 	Type=mat4, 		Array_size{constant}=MAX_BONES;
	Name=boneCount, Type=float;
]]

#code_block:

void main()
{	
	vec4 pos;
#ifdef HAS_SKIN
	mat4 boneTransform = skinned_ubo.bones[int(inBoneId.x)] * inWeights.x;
	boneTransform += skinned_ubo.bones[int(inBoneId.y)] * inWeights.y;
	boneTransform += skinned_ubo.bones[int(inBoneId.z)] * inWeights.z;
	boneTransform += skinned_ubo.bones[int(inBoneId.w)] * inWeights.w;

	mat4 normalTransform = mesh_ubo.modelMatrix * boneTransform;
#else	
	mat4 normalTransform = mesh_ubo.modelMatrix;
#endif
	pos = normalTransform * inPos;

    // inverse-transpose for non-uniform scaling - expensive computations here - maybe remove this?
	outNormal = normalize(transpose(inverse(mat3(normalTransform))) * inNormal);    

	outPos = pos.xyz / pos.w;	// perspective divide
	
	gl_Position = camera_ubo.mvp * vec4(outPos, 1.0);
	outUv = inUv;
}

#end_code_block

##end_stage

////////////////////////////////////
##stage: Fragment

#constant: Name=sampleCount, Type=int, Value=1024;

// outputs to the deferred buffers
#output: Name=Position, Type=vec4;
#output: Name=Colour, 	Type=vec4;
#output: Name=Normal, 	Type=vec4;
#output: Name=Pbr, 		Type=vec2;
#output: Name=Emissive, Type=vec4;

#import_material_sampler: Name=baseColourMap, 	Type=2D_Sampler, 	Variant=HAS_BASECOLOUR,			GroupId=1;
#import_material_sampler: Name=normalMap, 		Type=2D_Sampler,	Variant=HAS_NORMAL,				GroupId=1;
#import_material_sampler: Name=mrMap, 			Type=2D_Sampler,	Variant=HAS_METALLICROUGHNESS,	GroupId=1;
#import_material_sampler: Name=emissiveMap, 	Type=2D_Sampler,	Variant=HAS_EMISSIVE,			GroupId=1;
#import_material_sampler: Name=aoMap, 			Type=2D_Sampler,	Variant=HAS_OCCLUSION,			GroupId=1;

#push_constant: Name=MaterialPush, id=material;
[[
	Name=baseColourFactor,	Type=vec4;
	Name=emissiveFactor,	Type=vec4;
	Name=diffuseFactor,		Type=vec4;
	Name=specularFactor,	Type=vec4;
	Name=metallicFactor,	Type=float;
	Name=roughnessFactor,	Type=float;
	Name=alphaMask,			Type=float;
	Name=alphaMaskCutoff,	Type=float;
]]

#code_block:

#define EPSILON 0.0000001

float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular)
{
	float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
	float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
	
	if (perceivedSpecular < 0.04) {
		return 0.0;
	}
	
	float a = 0.04;
	float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - 0.04) + perceivedSpecular - 2.0 * 0.04;
	float c = 0.04 - perceivedSpecular;
	float D = max(b * b - 4.0 * a * c, 0.0);
	
	return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}

// The most copied function in the world! From here: http://www.thetenthplanet.de/archives/1180
vec3 peturbNormal(vec2 tex_coord)
{
	// convert normal to -1, 1 coord system
	vec3 tangentNormal = texture(normalMap, tex_coord).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inPos);			// edge1
	vec3 q2 = dFdy(inPos);			// edge2
	vec2 st1 = dFdx(tex_coord);		// uv1
	vec2 st2 = dFdy(tex_coord);		// uv2

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main()
{
	// albedo
	vec4 baseColour;
	
	if (material.alphaMask == 0.0) {
#ifdef HAVE_BASECOLOUR 
		baseColour = texture(baseColourMap, inUv) * material.baseColourFactor;
#else
		baseColour = material.baseColourFactor;
#endif
	}
	if (baseColour.a < material.alphaMaskCutoff) {
		discard;
	}	
	
	// normal
	vec3 normal; 
#ifdef HAS_NORMAL
	normal = peturbNormal(inUv);
#else
	normal = normalize(inNormal);
#endif

	outNormal = vec4(normal, 1.0);

	// diffuse - based on metallic and roughness
	float roughness = 0.0;
	float metallic = 0.0;
	
#ifdef HAS_SPECULAR_GLOSSINESS
	roughness = material.roughnessFactor;
	metallic = material.metallicFactor;

#ifdef HAS_METALLICROUGHNESS
	vec4 mrSample = texture(mrMap, inUv);
	roughness = clamp(mrSample.g * roughness, 0.0, 1.0);
	metallic = mrSample.b * metallic;
#else
	roughness = clamp(roughness, 0.04, 1.0);
	metallic = clamp(metallic, 0.0, 1.0);
#endif

#else
	// Values from specular glossiness workflow are converted to metallic roughness
	vec4 diffuse;
	vec3 specular;

#ifdef HAS_METALLICROUGHNESS
	roughness = 1.0 - texture(mrMap, inUv).a;
	specular = texture(mrMap, inUv).rgb;
#else
	roughness = 0.0;
	specular = vec3(0.0);
#endif
	
#ifdef HAS_BASECOLOUR
	diffuse = texture(baseColourMap, inUv);
#else
	diffuse = material.baseColorFactor;
#endif

	float maxSpecular = max(max(specular.r, specular.g), specular.b);

	// Convert metallic value from specular glossiness inputs
	metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);
	
	const float minRoughness = 0.04;	// this could be user defined?
	vec3 baseColourDiffusePart = diffuse.rgb * ((1.0 - maxSpecular) / (1 - minRoughness) / max(1 - metallic, EPSILON)) * material.diffuseFactor.rgb;
	vec3 baseColourSpecularPart = specular - (vec3(minRoughness) * (1 - metallic) * (1 / max(metallic, EPSILON))) * material.specularFactor.rgb;
	baseColour = vec4(mix(baseColourDiffusePart, baseColourSpecularPart, metallic * metallic), diffuse.a);
 #endif
 
	outColour = baseColour;
	outPbr = vec2(metallic, roughness);

	// ao
	float ambient = 1.0;
#ifdef HAS_AO
    ambient = texture(aoMap, inUv).x;
#endif
	outColour.a = ambient;

	// emmisive
	vec3 emissive;
#ifdef HAS_EMISSIVE
    emissive = texture(emissiveMap, inUv).rgb;
	emissive *= material.emissiveFactor.rgb;
#else
   	emissive = material.emissiveFactor.rgb;
#endif
	outEmissive = vec4(emissive, 1.0);
	
	outPosition = vec4(inPos, 1.0);
}

#end_code_block

##end_stage
