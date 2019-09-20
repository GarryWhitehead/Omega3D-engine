#include "GltfModel.h"

#include "Utility/FileUtil.h"
#include "Utility/logger.h"
#include "utility/String.h"

#include "Types/ComponentTypes.h"
#include "Types/Object.h"

#include "Core/World.h"

namespace OmegaEngine
{

void GltfModel::getAttributeData(const cgltf_attribute* attrib, uint8_t* base, size_t& stride)
{
	const cgltf_accessor* accessor = attrib->data;
	base = static_cast<uint8_t*>(accessor->buffer_view->buffer->data);    // total data blob
	stride = accessor->buffer_view->stride;                               // the size of each sub blob
	if (!stride)
	{
		stride = accessor->stride;
	}
	assert(stride);
}

OEMaths::vec3f GltfModel::tokenToVec3(Util::String str)
{
	OEMaths::vec3f output;
	auto split = str.split(' ');
	assert(split.size() == 3);
	return OEMaths::vec3f(split[0].toFloat(), split[1].toFloat(), split[2].toFloat());
}

bool GltfModel::prepareExtensions(const cgltf_extras& extras, cgltf_data& data, ExtensionData& extensions)
{
	// first check whther there are any extensions
	cgltf_size extSize;
	cgltf_copy_extras_json(&data, &extras, nullptr, &extSize);
	if (!extSize)
	{
		return false;
	}

	// alloc mem for the json extension data
	char* jsonData = new char[extSize + 1];

	cgltf_result res = cgltf_copy_extras_json(&data, &extras, jsonData, &extSize);
	if (!res)
	{
		LOGGER_ERROR("Unable to prepare extension data. Error code: %d\n", res);
		return false;
	}

	jsonData[extSize] = '\0';

	// create json file from data blob - using cgltf implementation here
	jsmn_parser jsonParser;
	jsmn_init(&jsonParser);
	const int tokenCount = jsmn_parse(&jsonParser, jsonData, extSize, nullptr, 0);
	if (tokenCount < 1)
	{
		LOGGER_ERROR("Unable to parse extensions json file.\n");
		return false;
	}

	// we know the token size, so allocate memory for this and get the tokens
	std::vector<jsmntok_t> tokenData;
	jsmn_parse(&jsonParser, jsonData, extSize, tokenData.data(), tokenCount);

	// now convert everything to a string for easier storage
	std::vector<Util::String> temp;
	for (const jsmntok_t& token : tokenData)
	{
		Util::String str(&jsonData[token.start], token.end - token.start);
		temp.emplace_back(str);
	}

	// should be divisible by 2 otherwise we will overflow when creating the output
	if ((temp.size() % 2) != 0)
	{
		LOGGER_ERROR("Error while parsing tokens. Invalid size.\n");
		return false;
	}

	// create the output - <extension name, value>
	for (size_t i = 0; i < temp.size(); i += 2)
	{
		extensions.emplace(temp[i], temp[i + 1]);
	}

	return true;
}

void GltfModel::lineariseRecursive(cgltf_node& node, size_t index)
{
	linearisedNodes.emplace(&node, index++);

	cgltf_node** childEnd = node.children + node.children_count;
	for (cgltf_node* const* child = node.children; child < childEnd; ++child)
	{
		lineariseRecursive(**child, index);
	}
}

void GltfModel::lineariseNodes(cgltf_data* data)
{
	size_t index = 0;

	cgltf_scene* sceneEnd = data->scenes + data->scenes_count;
	for (const cgltf_scene* scene = data->scenes; scene < sceneEnd; ++scene)
	{
		cgltf_node* const* nodeEnd = scene->nodes + scene->nodes_count;
		for (cgltf_node* const* node = scene->nodes; node < nodeEnd; ++node)
		{
			lineariseRecursive(**node, index);
		}
	}
}

bool GltfModel::load(Util::String filename)
{
	// no additional options required
	cgltf_options options = {};
	cgltf_data* gltfData;

	cgltf_result res = cgltf_parse_file(&options, filename.c_str(), &gltfData);
	if (res != cgltf_result_success)
	{
		LOGGER_ERROR("Unable to open gltf file %s. Error code: %d\n", filename.c_str(), res);
		return false;
	}

	// the buffers need parsing separately
	res = cgltf_load_buffers(&options, gltfData, filename.c_str());
	if (res != cgltf_result_success)
	{
		LOGGER_ERROR("Unable to open gltf file data for %s. Error code: %d\n", filename.c_str(), res);
		return false;
	}

	// start by linearising the nodes. This is ued for matching nodes with an index value
	// in skeleton and animation
	lineariseNodes(gltfData);

	cgltf_scene* sceneEnd = gltfData->scenes + gltfData->scenes_count;

	// for each scene, visit each node in that scene
	for (const cgltf_scene* scene = gltfData->scenes; scene < sceneEnd; ++scene)
	{
		cgltf_node* const* nodeEnd = scene->nodes + scene->nodes_count;
		for (cgltf_node* const* node = scene->nodes; node < nodeEnd; ++node)
		{
			ModelNode newNode;
			if (!newNode.prepare(**node, OEMaths::mat4f{}))
			{
				return false;
			}
		}
	}
}

void buildRecursive(std::unique_ptr<GltfModel::ModelNode>& node, Object* parentObj, World& world)
{
	if (node->hasMesh())
	{
		auto& meshManager = world.getMeshManager();
		meshManager.addMesh(node->getMesh());

		// TODO: obtain these parameters from the config once it has been refactored
		parentObj->addComponent<ShadowComponent>(0.0f, 1.25f, 1.75f);
	}
	if (node->hasTransform())
	{
		auto& transManager = world.getTransManager();
		transManager->addTransform(node->getTransform(), parentObj);
	}

	if (node->hasSkin())
	{
		SkinnedComponent comp;
		comp.setIndex(node->getSkinIndex(), skinIndex);
		parentObj->addComponent<SkinnedComponent>(comp);
	}
	if (node->isJoint())
	{
		auto& transManager = world.getTransManager();
		transManager->addSkeleton(node->getJoint(), node->isSkeletonRoot(), parentObj);
	}
	if (node->hasAnimation())
	{
		auto& animManager = world.getAnimManager();
		size_t bufferIndex = node->getAnimIndex + animIndex;

		for (auto& index : node->getChannelIndices())
		{
			animManager.addAnimation(bufferIndex, parentObject);
		}
	}

	if (node->hasChildren())
	{
		for (uint32_t i = 0; i < node->childCount(); ++i)

			auto child = objectManager->createChildObject(parentObject);
		buildRecursive(node->getChildNode(i), child, matIndex, skinIndex, animIndex);
	}
}
}    // namespace OmegaEngine

void build(GltfModel::Model& model, World& world, Object& obj)
{
	// ** extract all the data from the gltf blob and add to the appropiate manager **
	// materials and their textures
	auto& matManager = world.getMatManager();
	for (auto& mat : model.materials)
	{
		matManager.addMaterial(mat, model.images);
	}

	// skins
	auto& transManager = world.getTransManager();
	for (auto& skin : model.skins)
	{
		transManager->addSkin(skin);
	}

	// animations
	auto& animManager = world.getAnimManager();
	for (auto& anim : model.animations)
	{
		animManager.addAnim(anim);
	}

	// go through each node and add as a child of the parent
	for (auto& node : model.nodes)
	{
		Object child = ObjectManager::createObject();

		buildRecursive(node, child, world);
		obj.addChild(child);
	}
}

}    // namespace OmegaEngine

namespace GltfParser
{

std::unique_ptr<OmegaEngine::ModelImage> image(tinygltf::Model& model, tinygltf::Texture& texture)
{
	auto modelImage = std::make_unique<OmegaEngine::ModelImage>();

	// map to temporary storage until transferred to the asset manager
	tinygltf::Image image = model.images[texture.source];
	modelImage->map(image.width, image.height, image.image.data());

	// not guarenteed to have a sampler
	if (texture.sampler > -1)
	{
		tinygltf::Sampler gltfSampler = model.samplers[texture.sampler];

		modelImage->addSampler(gltfSampler.wrapS, gltfSampler.minFilter);
	}

	return std::move(modelImage);
}

std::unique_ptr<OmegaEngine::ModelMesh> mesh(tinygltf::Model& model, tinygltf::Node& node)
{
	auto modelMesh = std::make_unique<OmegaEngine::ModelMesh>();

	const tinygltf::Mesh& mesh = model.meshes[node.mesh];

	uint32_t localVertexOffset = 0;
	uint32_t localIndexOffset = 0;

	// get all the primitives associated with this mesh
	for (uint32_t i = 0; i < mesh.primitives.size(); ++i)
	{
		uint32_t vertexStart = static_cast<uint32_t>(modelMesh->vertices.size());
		const tinygltf::Primitive& primitive = mesh.primitives[i];

		// if this primitive has no indices associated with it, no point in continuing
		if (primitive.indices < 0)
		{
			continue;
		}

		// lets get the vertex data....
		if (primitive.attributes.find("POSITION") == primitive.attributes.end())
		{
			LOGGER_ERROR("Problem parsing gltf file. Appears to be missing position attribute. Exiting....");
		}

		const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
		const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
		const float* posBuffer = reinterpret_cast<const float*>(
		    &(model.buffers[posBufferView.buffer].data[posAccessor.byteOffset + posBufferView.byteOffset]));

		// find normal data
		const float* normBuffer = nullptr;
		if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
		{
			const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
			const tinygltf::BufferView& normBufferView = model.bufferViews[normAccessor.bufferView];
			normBuffer = reinterpret_cast<const float*>(
			    &(model.buffers[normBufferView.buffer].data[normAccessor.byteOffset + normBufferView.byteOffset]));
		}

		// and parse uv data - there can be two tex coord buffers, we need to check for both
		const float* uvBuffer0 = nullptr;
		if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
		{
			const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
			const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
			uvBuffer0 = reinterpret_cast<const float*>(
			    &(model.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
		}

		const float* uvBuffer1 = nullptr;
		if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end())
		{
			const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
			const tinygltf::BufferView& uvBufferView = model.bufferViews[uvAccessor.bufferView];
			uvBuffer1 = reinterpret_cast<const float*>(
			    &(model.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset]));
		}

		// check whether this model has skinning data - joints first
		const uint16_t* jointBuffer = nullptr;
		if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end())
		{
			modelMesh->skinned = true;
			const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
			const tinygltf::BufferView& jointBufferView = model.bufferViews[jointAccessor.bufferView];
			jointBuffer = reinterpret_cast<const uint16_t*>(
			    &(model.buffers[jointBufferView.buffer].data[jointAccessor.byteOffset + jointBufferView.byteOffset]));
		}

		// and then weights. It must contain both to for the data to be used for animations
		const float* weightBuffer = nullptr;
		if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end())
		{
			const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
			const tinygltf::BufferView& weightBufferView = model.bufferViews[weightAccessor.bufferView];
			weightBuffer = reinterpret_cast<const float*>(&(
			    model.buffers[weightBufferView.buffer].data[weightAccessor.byteOffset + weightBufferView.byteOffset]));
		}

		// get the min and max values for this primitive TODO:: FIX THIS!
		OEMaths::vec3f primMin{
			0.0f, 0.0f, 0.0f
		};    // { posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2] };
		OEMaths::vec3f primMax{
			0.0f, 0.0f, 0.0f
		};    //{ posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2] };

		// now convert the data to a form that we can use with Vulkan - skinned and non-skinned meshes treated separately
		for (uint32_t j = 0; j < posAccessor.count; ++j)
		{
			OmegaEngine::ModelMesh::Vertex vertex;
			vertex.position = OEMaths::vec4f(OEMaths::vec3f(posBuffer), 1.0f);
			posBuffer += 3;

			if (normBuffer)
			{
				vertex.normal = OEMaths::vec3f(normBuffer);
				normBuffer += 3;
			}
			if (uvBuffer0)
			{
				vertex.uv0 = OEMaths::vec2f(uvBuffer0);
				uvBuffer0 += 2;
			}
			if (uvBuffer1)
			{
				vertex.uv1 = OEMaths::vec2f(uvBuffer1);
				uvBuffer1 += 2;
			}

			// if we have skin data, also convert this to a palatable form
			if (weightBuffer && jointBuffer)
			{
				vertex.joint = OEMaths::vec4f(jointBuffer);
				vertex.weight = OEMaths::vec4f(weightBuffer);
				jointBuffer += 4;
				weightBuffer += 4;
			}

			modelMesh->vertices.push_back(vertex);
		}

		localVertexOffset += static_cast<uint32_t>(posAccessor.count);

		// Now obtain the indicies data from the gltf file
		const tinygltf::Accessor& indAccessor = model.accessors[primitive.indices];
		const tinygltf::BufferView& indBufferView = model.bufferViews[indAccessor.bufferView];
		const tinygltf::Buffer& indBuffer = model.buffers[indBufferView.buffer];

		uint32_t indexCount = static_cast<uint32_t>(indAccessor.count);

		// the indicies can be stored in various formats - this can be determined from the component type in the accessor
		switch (indAccessor.componentType)
		{
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
		{
			parseIndices<uint32_t>(indAccessor, indBufferView, indBuffer, modelMesh->indices, vertexStart);
			break;
		}
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
		{
			parseIndices<uint16_t>(indAccessor, indBufferView, indBuffer, modelMesh->indices, vertexStart);
			break;
		}
		case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
		{
			parseIndices<uint8_t>(indAccessor, indBufferView, indBuffer, modelMesh->indices, vertexStart);
			break;
		}
		default:
			throw std::runtime_error("Unable to parse indices data. Unsupported accessor component type.");
		}

		modelMesh->primitives.push_back({ localIndexOffset, indexCount, static_cast<int32_t>(primitive.material) });
		localIndexOffset += indexCount;
	}

	return std::move(modelMesh);
}

std::unique_ptr<OmegaEngine::ModelMaterial> material(tinygltf::Material& gltfMaterial)
{
	auto modelMat = std::make_unique<OmegaEngine::ModelMaterial>();

	modelMat->name = gltfMaterial.name;

	// go through each material type and see if they exsist - we are only saving the index
	if (gltfMaterial.values.find("baseColorTexture") != gltfMaterial.values.end())
	{
		modelMat->textures.baseColour = gltfMaterial.values["baseColorTexture"].TextureIndex();
		modelMat->uvSets.baseColour = gltfMaterial.values["baseColorTexture"].TextureTexCoord();
	}
	if (gltfMaterial.values.find("metallicRoughnessTexture") != gltfMaterial.values.end())
	{
		modelMat->textures.metallicRoughness = gltfMaterial.values["metallicRoughnessTexture"].TextureIndex();
		modelMat->uvSets.metallicRoughness = gltfMaterial.values["metallicRoughnessTexture"].TextureTexCoord();
	}
	if (gltfMaterial.values.find("baseColorFactor") != gltfMaterial.values.end())
	{
		modelMat->factors.baseColour = OEMaths::vec4f(gltfMaterial.values["baseColorFactor"].ColorFactor().data());
	}
	if (gltfMaterial.values.find("metallicFactor") != gltfMaterial.values.end())
	{
		modelMat->factors.metallic = static_cast<float>(gltfMaterial.values["metallicFactor"].Factor());
	}
	if (gltfMaterial.values.find("roughnessFactor") != gltfMaterial.values.end())
	{
		modelMat->factors.roughness = static_cast<float>(gltfMaterial.values["roughnessFactor"].Factor());
	}

	// any additional textures?
	if (gltfMaterial.additionalValues.find("normalTexture") != gltfMaterial.additionalValues.end())
	{
		modelMat->textures.normal = gltfMaterial.additionalValues["normalTexture"].TextureIndex();
		modelMat->uvSets.normal = gltfMaterial.additionalValues["normalTexture"].TextureTexCoord();
	}
	if (gltfMaterial.additionalValues.find("emissiveTexture") != gltfMaterial.additionalValues.end())
	{
		modelMat->textures.emissive = gltfMaterial.additionalValues["emissiveTexture"].TextureIndex();
		modelMat->uvSets.emissive = gltfMaterial.additionalValues["emissiveTexture"].TextureTexCoord();
	}
	if (gltfMaterial.additionalValues.find("occlusionTexture") != gltfMaterial.additionalValues.end())
	{
		modelMat->textures.occlusion = gltfMaterial.additionalValues["occlusionTexture"].TextureIndex();
		modelMat->uvSets.occlusion = gltfMaterial.additionalValues["occlusionTexture"].TextureTexCoord();
	}

	// check for aplha modes
	if (gltfMaterial.additionalValues.find("alphaMode") != gltfMaterial.additionalValues.end())
	{
		tinygltf::Parameter param = gltfMaterial.additionalValues["alphaMode"];
		modelMat->factors.mask = param.string_value;
	}
	if (gltfMaterial.additionalValues.find("alphaCutOff") != gltfMaterial.additionalValues.end())
	{
		modelMat->factors.alphaMaskCutOff = static_cast<float>(gltfMaterial.additionalValues["alphaCutOff"].Factor());
	}
	if (gltfMaterial.additionalValues.find("emissiveFactor") != gltfMaterial.additionalValues.end())
	{
		modelMat->factors.emissive =
		    OEMaths::vec3f(gltfMaterial.additionalValues["emissiveFactor"].ColorFactor().data());
	}

	// check for extensions
	auto extension = gltfMaterial.extensions.find("KHR_materials_pbrSpecularGlossiness");
	if (extension != gltfMaterial.extensions.end())
	{
		if (extension->second.Has("specularGlossinessTexture"))
		{
			auto index = extension->second.Get("specularGlossinessTexture").Get("index");
			modelMat->textures.metallicRoughness = index.Get<int>();
			modelMat->usingSpecularGlossiness = true;

			auto uv_index = extension->second.Get("specularGlossinessTexture").Get("texCoord");
			modelMat->uvSets.specularGlossiness = uv_index.Get<int>();
		}
		if (extension->second.Has("diffuseTexture"))
		{
			auto index = extension->second.Get("diffuseTexture").Get("index");
			modelMat->textures.baseColour = index.Get<int>();
			modelMat->usingSpecularGlossiness = true;

			auto uvIndex = extension->second.Get("diffuseTexture").Get("texCoord");
			modelMat->uvSets.diffuse = uvIndex.Get<int>();
		}
		if (extension->second.Has("diffuseFactor"))
		{
			auto factor = extension->second.Get("diffuseFactor");
			auto value = factor.Get(0);
			float x = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();
			value = factor.Get(1);
			float y = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();
			value = factor.Get(2);
			float z = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();
			value = factor.Get(3);
			float w = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();

			modelMat->factors.diffuse = OEMaths::vec4f(x, y, z, w);
			modelMat->usingSpecularGlossiness = true;
		}
		if (extension->second.Has("specularFactor"))
		{
			auto factor = extension->second.Get("specularFactor");
			auto value = factor.Get(0);
			float x = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();
			value = factor.Get(1);
			float y = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();
			value = factor.Get(2);
			float z = value.IsNumber() ? (float)value.Get<double>() : (float)value.Get<int>();

			modelMat->factors.specular = OEMaths::vec3f(x, y, z);
			modelMat->usingSpecularGlossiness = true;
		}
	}

	return std::move(modelMat);
}

std::unique_ptr<OmegaEngine::ModelAnimation> animation(tinygltf::Model& gltfModel, tinygltf::Animation& anim,
                                                       std::unique_ptr<GltfModel::Model>& model, const uint32_t index)
{
	auto modelAnim = std::make_unique<OmegaEngine::ModelAnimation>();

	modelAnim->name = anim.name.c_str();

	// get channel data
	uint32_t channelIndex = 0;
	for (auto& source : anim.channels)
	{
		ModelAnimation::Channel channel;

		channel.pathType = source.target_path;
		channel.samplerIndex = source.sampler;

		// set animation flag on relevant node
		auto node = model->getNode(source.target_node);
		assert(node != nullptr);
		node->setAnimationIndex(index, channelIndex++);

		modelAnim->channels.emplace_back(channel);
	}

	// get sampler data
	for (auto& sampler : anim.samplers)
	{
		ModelAnimation::Sampler samplerInfo;
		samplerInfo.interpolation = sampler.interpolation;

		tinygltf::Accessor timeAccessor = gltfModel.accessors[sampler.input];
		tinygltf::BufferView timeBufferView = gltfModel.bufferViews[timeAccessor.bufferView];
		tinygltf::Buffer timeBuffer = gltfModel.buffers[timeBufferView.buffer];

		// only supporting floats at the moment. This can be expaned on if the need arises...
		switch (timeAccessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
		{
			float* buffer = new float[timeAccessor.count];
			memcpy(buffer, &timeBuffer.data[timeAccessor.byteOffset + timeBufferView.byteOffset],
			       timeAccessor.count * sizeof(float));

			for (uint32_t i = 0; i < timeAccessor.count; ++i)
			{
				samplerInfo.timeStamps.emplace_back(buffer[i]);
			}
			delete buffer;
			break;
		}
		default:
			LOGGER_ERROR("Unsupported component type used for time accessor.");
		}

		// time and end points
		for (auto input : samplerInfo.timeStamps)
		{
			if (input < modelAnim->start)
			{
				modelAnim->start = input;
			}
			if (input > modelAnim->end)
			{
				modelAnim->end = input;
			}
		}

		// get TRS data
		tinygltf::Accessor trsAccessor = gltfModel.accessors[sampler.output];
		tinygltf::BufferView trsBufferView = gltfModel.bufferViews[trsAccessor.bufferView];
		tinygltf::Buffer trsBuffer = gltfModel.buffers[trsBufferView.buffer];

		// again, for now, only supporting floats
		switch (trsAccessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
		{
			// all types will be converted to vec4 for ease of use
			switch (trsAccessor.type)
			{
			case TINYGLTF_TYPE_VEC3:
			{
				OEMaths::vec3f* buffer = reinterpret_cast<OEMaths::vec3f*>(
				    &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset]);
				for (uint32_t i = 0; i < trsAccessor.count; ++i)
				{
					samplerInfo.outputs.push_back(OEMaths::vec4f(buffer[i], 0.0f));
				}
				break;
			}
			case TINYGLTF_TYPE_VEC4:
			{
				samplerInfo.outputs.resize(trsAccessor.count);
				memcpy(samplerInfo.outputs.data(), &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset],
				       trsAccessor.count * sizeof(OEMaths::vec4f));
				break;
			}
			default:
				LOGGER_ERROR("Unsupported component type used for TRS accessor.");
			}
		}
		}

		modelAnim->samplers.emplace_back(samplerInfo);
	}

	return std::move(modelAnim);
}

std::unique_ptr<OmegaEngine::ModelTransform> transform(tinygltf::Node& node)
{
	auto transform = std::make_unique<OmegaEngine::ModelTransform>();

	// we will save the matrix and the decomposed form
	if (node.translation.size() == 3)
	{
		transform->translation =
		    OEMaths::vec3f{ (float)node.translation[0], (float)node.translation[1], (float)node.translation[2] };
	}
	if (node.scale.size() == 3)
	{
		transform->scale = OEMaths::vec3f{ (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] };
	}
	if (node.rotation.size() == 4)
	{
		OEMaths::quatf quat(node.rotation.data());
		transform->rotation = quat;
	}

	if (node.matrix.size() == 16)
	{
		transform->trsMatrix = OEMaths::mat4f(node.matrix.data());
	}

	return std::move(transform);
}

std::unique_ptr<OmegaEngine::ModelSkin> skin(tinygltf::Model& gltfModel, tinygltf::Skin& skin,
                                             std::unique_ptr<GltfModel::Model>& model, uint32_t skinIndex)
{
	auto modelSkin = std::make_unique<OmegaEngine::ModelSkin>();

	modelSkin->name = skin.name.c_str();

	// Is this the skeleton root node?
	if (skin.skeleton > -1)
	{
		auto node = model->getNode(skin.skeleton);
		assert(node != nullptr);
		node->setSkeletonRootFlag();
	}

	// Does this skin have joint nodes?
	for (auto& jointIndex : skin.joints)
	{
		auto node = model->getNode(jointIndex);
		assert(node != nullptr);
		node->setJoint(skinIndex);
	}

	// get the inverse bind matricies, if there are any
	if (skin.inverseBindMatrices > -1)
	{
		tinygltf::Accessor accessor = gltfModel.accessors[skin.inverseBindMatrices];
		tinygltf::BufferView bufferView = gltfModel.bufferViews[accessor.bufferView];
		tinygltf::Buffer buffer = gltfModel.buffers[bufferView.buffer];

		modelSkin->invBindMatrices.resize(accessor.count);
		memcpy(modelSkin->invBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset],
		       accessor.count * sizeof(OEMaths::mat4f));
	}

	return std::move(modelSkin);
}
}    // namespace GltfParser

}    // namespace GltfModel
}    // namespace OmegaEngine
