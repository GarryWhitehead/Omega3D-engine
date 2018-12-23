#include "GltfParser.h"

#include "DataTypes/TextureType.h"
#include "Utility/FileUtil.h"
#include "Utility/logger.h"

namespace OmegaEngine
{
	
	GltfParser::GltfParser()
	{
	}


	GltfParser::~GltfParser()
	{
	}

	bool GltfParser::parse(const char *filename)
	{
		tinygltf::TinyGLTF loader;

		std::string err, warn;
		std::string ext;

		FileUtil::GetFileExtension(filename, ext);
		bool ret = false;
		if (ext.compare("glb") == 0) {
			ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
		}
		else {
			ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
		}

		// we can continue if we have just a warning for now
		if (!warn.empty()) {
			LOGGER_INFO("Problem whilst parsing gltf file: %s. Going to hope for the best...", warn);
			return false;
		}

		// Although, if there's an error of any kind, time to quit
		if (!err.empty()) {
			LOGGER_ERROR("Error whilst parsing gltf file: %s", err);
			return false;
		}
		if (!ret) {
			LOGGER_ERROR("Unable to load scene data from file: %s. No error message available.", filename);
			return false;
		}

		// parse all the model data from the gltf file....
		loadNode();
		loadMaterial();
		loadTextures();
		loadAnimation();
		loadSkin();

		return true;
	}

	void GltfParser::loadNode()
	{
		// we are going to parse the node recursively to get all the info required for the space
		tinygltf::Scene &scene = model.scenes[model.defaultScene];
		for (uint32_t i = 0; i < scene.nodes.size(); ++i) {

			tinygltf::Node node = model.nodes[scene.nodes[i]];
			parseNodeRecursive(-1, node);
		}
	}

	void GltfParser::parseNodeRecursive(uint32_t parentNode, tinygltf::Node& node)
	{
		GltfNodeInfo newNode;

		// set index of the new buffer which is where this will reside within the buffer
		if (nodeBuffer.empty()) {
			newNode.index = 0;
		}
		else {
			newNode.index = nodeBuffer.size();
		}

		newNode.parentIndex = parentNode;
		newNode.name = node.name.c_str();
		newNode.skinIndex = node.skin;

		// get the transforms associated with this node
		if (node.translation.size() == 3) {
			newNode.translation = OEMaths::convert_vec3((float*)node.translation.data());
		}
		if (node.scale.size() == 3) {
			newNode.scale = OEMaths::convert_vec3((float*)node.scale.data());
		}
		if (node.rotation.size() == 4) {
			OEMaths::quatf q = OEMaths::convert_quat((float*)node.rotation.data());
			newNode.rotation = OEMaths::quat_to_mat4(q);
		}
		if (node.matrix.size() == 16) {
			newNode.matrix = OEMaths::convert_mat4((float*)node.rotation.data());
		}

		// if this node has children, recursively extract their info
		if (!node.children.empty()) {
			for (uint32_t i = 0; node.children.size(); ++i) {
				parseNodeRecursive(newNode.index, model.nodes[node.children[i]]);
			}
		}

		// if the node has mesh data...
		if (node.mesh > -1) {
			tinygltf::Mesh mesh = model.meshes[node.mesh];

			GltfStaticMeshInfo staticMesh;

			// get all the primitives associated with this mesh
			for (uint32_t i = 0; i < mesh.primitives.size(); ++i) {

				tinygltf::Primitive primitive = mesh.primitives[i];

				// if this primitive has no indices associated with it, no point in continuing
				if (primitive.indices < 0) {
					continue;
				}

				// lets get the vertex data....
				if (primitive.attributes.find("POSITION") == primitive.attributes.end()) {
					LOGGER_ERROR("Problem parsing gltf file. Appears to be missing position attribute. Exiting....");
					throw std::runtime_error("Mesh data doesn't contain position attribute.");
				}

				tinygltf::Accessor posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
				tinygltf::BufferView posBufferView = model.bufferViews[posAccessor.bufferView];
				float *posBuffer = (float*)model.buffers[posBufferView.buffer].data[posAccessor.byteOffset + posBufferView.byteOffset];

				// find normal data
				float *normBuffer = nullptr;
				if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
					tinygltf::Accessor normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
					tinygltf::BufferView normBufferView = model.bufferViews[normAccessor.bufferView];
					normBuffer = (float*)model.buffers[normBufferView.buffer].data[normAccessor.byteOffset + normBufferView.byteOffset];
				}

				// and parse uv data
				float *uvBuffer = nullptr;
				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
					tinygltf::Accessor uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
					tinygltf::BufferView uvBufferView = model.bufferViews[uvAccessor.bufferView];
					uvBuffer = (float*)model.buffers[uvBufferView.buffer].data[uvAccessor.byteOffset + uvBufferView.byteOffset];
				}

				// check whether this model has skinning data - joints first
				uint16_t *jointBuffer = nullptr;
				if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
					tinygltf::Accessor jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
					tinygltf::BufferView jointBufferView = model.bufferViews[jointAccessor.bufferView];
					jointBuffer = (uint16_t*)model.buffers[jointBufferView.buffer].data[jointAccessor.byteOffset + jointBufferView.byteOffset];
				}

				// and then weights. It must contain both to for the data to be used for animations
				float *weightBuffer = nullptr;
				if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
					tinygltf::Accessor weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
					tinygltf::BufferView weightBufferView = model.bufferViews[weightAccessor.bufferView];
					weightBuffer = (float*)model.buffers[weightBufferView.buffer].data[weightAccessor.byteOffset + weightBufferView.byteOffset];
				}

				// get the min and max values for this primitive
				OEMaths::vec3f primMin{ posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2] };
				OEMaths::vec3f primMax{ posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2] };

				// now convert the data to a form that we can use with Vulkan
				for (uint32_t j = 0; j < posAccessor.count; ++j) {
					GltfVertex vertex;
					vertex.position = OEMaths::vec3_to_vec4(OEMaths::convert_vec3(&posBuffer[j * 3]), 1.0f);

					if (normBuffer) {
						vertex.normal = OEMaths::normalise_vec3(OEMaths::convert_vec3(&normBuffer[j * 3]));
					}


					if (uvBuffer) {
						vertex.uv = OEMaths::convert_vec2(&uvBuffer[j * 2]);
					}

					// if we have skin data, also convert this to a palatable form
					if (weightBuffer && jointBuffer) {
						vertex.joint = OEMaths::convert_vec4(&jointBuffer[j * 4]);
						vertex.weight = OEMaths::convert_vec4(&weightBuffer[j * 4]);
					}
					vertexBuffer.push_back(vertex);
				}

				// Now obtain the indicies data from the gltf file
				tinygltf::Accessor indAccessor = model.accessors[primitive.indices];
				tinygltf::BufferView indBufferView = model.bufferViews[indAccessor.bufferView];
				tinygltf::Buffer indBuffer = model.buffers[indBufferView.buffer];

				uint32_t indexCount = indAccessor.count;
				uint32_t indexOffset = indiciesBuffer.size();

				// the indicies can be stored in various formats - this can be determined from the component type in the accessor
				switch (indAccessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
					parseIndices<uint32_t>(indAccessor, indBufferView, indBuffer, indiciesBuffer, indexOffset);
					break;
				}
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
					parseIndices<uint16_t>(indAccessor, indBufferView, indBuffer, indiciesBuffer, indexOffset);
					break;
				}
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
					parseIndices<uint8_t>(indAccessor, indBufferView, indBuffer, indiciesBuffer, indexOffset);
					break;
				}
				default:
					throw std::runtime_error("Unable to parse indices data. Unsupported accessor component type.");
				}

				GltfPrimitiveInfo prim(indexOffset, indexCount, (uint32_t)primitive.material, primMin, primMax);
				staticMesh.primitives.push_back(prim);
			}

			meshBuffer.push_back(staticMesh);
			newNode.meshIndex = std::make_tuple(spaceId, meshBuffer.size() - 1);
		}

		if (parentNode > -1) {
			nodeBuffer[parentNode].children.push_back(newNode.index);
			nodeBuffer.push_back(newNode);
			linearBuffer.push_back(newNode);
		}
		else {
			nodeBuffer.push_back(newNode);
			linearBuffer.push_back(newNode);
		}
	}

	void GltfParser::loadMaterial()
	{
	
		for (tinygltf::Material mat : model.materials) {

			GltfMaterialInfo vkmat;
			// go through each material type and see if they exsist - we are only saving the index - we can then use this and the space id to get the required textures when needed
			if (mat.values.find("baseColorTexture") != mat.values.end()) {
				vkmat.textureIndicies.baseColor = mat.values["baseColorTexture"].TextureIndex();
			}
			if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
				vkmat.textureIndicies.metallicRoughness = mat.values["metallicRoughnessTexture"].TextureIndex();
			}
			if (mat.values.find("baseColorFactor") != mat.values.end()) {
				vkmat.baseColorFactor = mat.values["baseColorFactor"].Factor();
			}
			if (mat.values.find("metallicRoughnessFactor") != mat.values.end()) {
				vkmat.metallicFactor = mat.values["metallicRoughnessFactor"].Factor();
			}

			// any additional textures?
			if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
				vkmat.textureIndicies.normal = mat.additionalValues["normalTexture"].TextureIndex();
			}
			if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
				vkmat.textureIndicies.emissive = mat.additionalValues["emissiveTexture"].TextureIndex();
			}
			if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
				vkmat.textureIndicies.occlusion = mat.additionalValues["occlusionTexture"].TextureIndex();
			}

			// check for aplha modes
			if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
				tinygltf::Parameter param = mat.additionalValues["alphaMode"];
				if (param.string_value == "BLEND") {
					vkmat.alphaMode = GltfMaterialInfo::AlphaMode::Blend;
				}
				if (param.string_value == "MASK") {
					vkmat.alphaMode = GltfMaterialInfo::AlphaMode::Mask;
				}
			}
			if (mat.additionalValues.find("alphaCutOff") != mat.additionalValues.end()) {
				vkmat.alphaCutOff = mat.additionalValues["alphaCutOff"].Factor();
			}
			if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
				vkmat.emissiveFactor = OEMaths::convert_vec3((float*)mat.additionalValues["emssiveFactor"].ColorFactor().data());
			}

			// check for extensions
			/*if (!mat.extensions.empty()) {

				vkmat.extension = std::make_unique<MaterialExt>();
				if (mat.extensions.find("specularGlossinessTexture") != mat.extensions.end()) {
					vkmat.extension->specularGlossiness = mat.extensions["specularGlossinessTexture"].
				}
			}*/

			materialsBuffer.push_back(vkmat);
		}
	}

	void GltfParser::loadTextures()
	{
		for (const auto texture : model.textures) {

			MappedTexture mappedTex;
			const auto& image = model.images[texture.source];
			int imageSize = image.width * image.height;

			mappedTex.loadPngTexture(imageSize, image.image.data());
			textureBuffer.push_back(mappedTex);
		}
	}

	void GltfParser::loadAnimation()
	{
	
		for (tinygltf::Animation& anim : model.animations) {

			GltfAnimationInfo animInfo;
			animInfo.name = anim.name.c_str();

			// get channel data
			for (auto& source : anim.channels) {
				Channel channel;

				if (source.target_path == "rotation") {
					channel.pathType = Channel::PathType::Rotation;
				}
				if (source.target_path == "scale") {
					channel.pathType = Channel::PathType::Scale;
				}
				if (source.target_path == "translation") {
					channel.pathType = Channel::PathType::Translation;
				}
				if (source.target_path == "weights") {
					LOGGER_INFO("Channel path type weights not yet supported.");
					continue;
				}

				channel.samplerIndex = source.sampler;
				channel.nodeIndex = source.target_node;
				animInfo.channels.push_back(channel);
			}

			// get sampler data
			for (auto& sampler : anim.samplers) {
				Sampler samplerInfo;

				if (sampler.interpolation == "LINEAR") {
					samplerInfo.interpolationType = Sampler::InerpolationType::Linear;
				}
				if (sampler.interpolation == "STEP") {
					samplerInfo.interpolationType = Sampler::InerpolationType::Step;
				}
				if (sampler.interpolation == "CUBICSPLINE") {
					samplerInfo.interpolationType = Sampler::InerpolationType::CubicSpline;
				}

				tinygltf::Accessor timeAccessor = model.accessors[sampler.input];
				tinygltf::BufferView timeBufferView = model.bufferViews[timeAccessor.bufferView];
				tinygltf::Buffer timeBuffer = model.buffers[timeBufferView.buffer];

				// only supporting floats at the moment. This can be expaned on if the need arises...
				switch (timeAccessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_FLOAT: {
					float* buffer = new float[timeAccessor.count];
					memcpy(buffer, &timeBuffer.data[timeAccessor.byteOffset + timeBufferView.byteOffset], timeAccessor.count * sizeof(float));

					for (uint32_t i = 0; i < timeAccessor.count; ++i) {
						samplerInfo.inputs.push_back(buffer[i]);
					}
					delete buffer;
					break;
				}
				default:
					LOGGER_ERROR("Unsupported component type used for time accessor.");
					throw std::runtime_error("Unsupported component type whilst parsing gltf file.");
				}

				// time and end points
				for (auto input : samplerInfo.inputs) {
					if (input < animInfo.start) {
						animInfo.start = input;
					}
					if (input > animInfo.end) {
						animInfo.end = input;
					}
				}

				// get TRS data
				tinygltf::Accessor trsAccessor = model.accessors[sampler.output];
				tinygltf::BufferView trsBufferView = model.bufferViews[trsAccessor.bufferView];
				tinygltf::Buffer trsBuffer = model.buffers[trsBufferView.buffer];

				// again, for now, only supporting floats
				switch (trsAccessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_FLOAT: {

					// all types will be converted to vec4 for ease of use
					switch (trsAccessor.type) {
					case TINYGLTF_TYPE_VEC3: {
						OEMaths::vec3f* buffer = new OEMaths::vec3f[trsAccessor.count];
						memcpy(buffer, &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset], trsAccessor.count * sizeof(OEMaths::vec3f));
						for (uint32_t i = 0; i < trsAccessor.count; ++i) {
							samplerInfo.outputs.push_back(OEMaths::vec4f(buffer[i], 1.0f));
						}
						delete buffer;
						break;
					}
					case TINYGLTF_TYPE_VEC4: {
						OEMaths::vec4f* buffer = new OEMaths::vec4f[trsAccessor.count];
						memcpy(buffer, &trsBuffer.data[trsAccessor.byteOffset + trsBufferView.byteOffset], trsAccessor.count * sizeof(OEMaths::vec4f));
						for (uint32_t i = 0; i < trsAccessor.count; ++i) {
							samplerInfo.outputs.push_back(buffer[i]);
						}
						delete buffer;
						break;
					}
					default:
						LOGGER_ERROR("Unsupported component type used for TRS accessor.");
						throw std::runtime_error("Unsupported component type whilst parsing gltf file.");
					}
				}
				}

				animInfo.samplers.push_back(samplerInfo);
			}

			animBuffer.push_back(animInfo);
		}
	}

	void GltfParser::loadSkin()
	{
		for (tinygltf::Skin& skin : model.skins) {
			GltfSkinInfo skinInfo;
			skinInfo.name = skin.name.c_str();

			// Is this the skeleton root node?
			if (skin.skeleton > -1) {
				skinInfo.skeletonIndex = skin.skeleton;
			}

			// Does this skin have joint nodes?
			for (auto& jointIndex : skin.joints) {

				// we will check later if this node actually exsists
				skinInfo.joints.push_back(jointIndex);
			}

			// get the inverse bind matricies, if there are any
			if (skin.inverseBindMatrices > -1) {
				tinygltf::Accessor accessor = model.accessors[skin.inverseBindMatrices];
				tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
				tinygltf::Buffer buffer = model.buffers[bufferView.buffer];

				skinInfo.invBindMatrices.resize(accessor.count);
				memcpy(skinInfo.invBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(OEMaths::mat4f));
			}

			skinBuffer.push_back(skinInfo);
		}
	}

	void GltfParser::loadLights()
	{
		// to add....
	}

	void GltfParser::loadEnvironment()
	{
		// to add extra.....

	}
}
