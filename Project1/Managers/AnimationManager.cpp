#include "AnimationManager.h"

#include "Managers/TransformManager.h"
#include "ComponentInterface/ObjectManager.h"
#include "Utility/logger.h"
#include "DataTypes/Object.h"

namespace OmegaEngine
{

	AnimationManager::AnimationManager()
	{
	}


	AnimationManager::~AnimationManager()
	{
	}

	void AnimationManager::addGltfAnimation(tinygltf::Model& model, Object& obj)
	{

		for (tinygltf::Animation& anim : model.animations) {

			AnimationInfo animInfo;
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

			animationBuffer.push_back(animInfo);

			// add animation component to object for this particular anim
			obj.add_manager<AnimationManager>(animationBuffer.size() - 1);
		}

		 
	}

	void AnimationManager::addGltfSkin(tinygltf::Model& model)
	{
		for (tinygltf::Skin& skin : model.skins) {
			SkinInfo skinInfo;
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

	void AnimationManager::update_recursive(std::unique_ptr<TransformManager>& transform_man, std::unique_ptr<ObjectManager>& obj_manager, uint32_t anim_index, Object& obj)
	{
		
		uint32_t transform_index = obj.get_manager_index<TransformManager>();

		OEMaths::mat4f mat = transform_man->get_transform(transform_index);

		// prepare fianl output matrices buffer
		uint32_t joint_size = std::min(skinBuffer[anim_index].joints.size(), MAX_NUM_JOINTS);
		skinBuffer[anim_index].joint_matrices.resize(joint_size);

		// transform to local space
		OEMaths::mat4f inv_mat = OEMaths::inverse(mat);


		for (uint32_t i = 0; i < joint_size; ++i) {
			Object joint_obj = skinBuffer[anim_index].joints[i];

			uint32_t joint_index = joint_obj.get_manager_index<TransformManager>();
			OEMaths::mat4f joint_mat = transform_man->get_transform(joint_index) * skinBuffer[anim_index].invBindMatrices[i];

			// transform joint to local (joint) space
			skinBuffer[anim_index].joint_matrices[i] = inv_mat * joint_mat;
		}

		// now update all child nodes too - TODO: do this without recursion
		auto& children = obj.get_children();

		for (auto& child : children) {
			update_recursive(transform_man, obj_manager, child.get_manager_index<AnimationManager>(), child);
		}
	
	}

	void AnimationManager::update(std::unique_ptr<TransformManager>& transform_man, std::unique_ptr<ObjectManager>& obj_manager)
	{
		auto object_list = obj_manager->get_objects_list();

		for (auto obj : object_list) {

			update_recursive(transform_man, obj_manager, obj.second.get_manager_index<AnimationManager>(), obj.second);
		}

			
	}
}
