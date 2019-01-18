#pragma once

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "DataTypes/object.h"

#include "ComponentInterface/ComponentManagerBase.h"
#include "tiny_gltf.h"

#define MAX_NUM_JOINTS 128

namespace OmegaEngine
{
	// forward declerations
	class Object;
	class TransformManager;

	class AnimationManager : public ComponentManagerBase
	{

	public:

		// skins
		struct SkinInfo
		{
			const char* name;
			uint32_t skeletonIndex;

			std::vector<Object> joints;
			std::vector<OEMaths::mat4f> invBindMatrices;
			std::vector<OEMaths::mat4f> joint_matrices;
		};


		// animation
		struct Sampler
		{
			enum class InerpolationType
			{
				Linear,
				Step,
				CubicSpline,
				Count
			} interpolationType;


			std::vector<float> inputs;
			std::vector<OEMaths::vec4f> outputs;
		};

		struct Channel
		{
			enum class PathType
			{
				Translation,
				Rotation,
				Scale,
				Count
			} pathType;

			uint32_t nodeIndex;
			uint32_t samplerIndex;

		};

		struct AnimationInfo
		{
			const char* name;
			float start;
			float end;
			std::vector<Sampler> samplers;
			std::vector<Channel> channels;

		};

		AnimationManager();
		~AnimationManager();

		void addGltfAnimation(tinygltf::Model& model, Object& obj);
		void addGltfSkin(tinygltf::Model& model);

		void update(std::unique_ptr<TransformManager>& transform_man);

	private:

		// a list of all objects associated with this manager and their position within the main data buffer
		std::unordered_map<Object, uint32_t, HashObject> objects;

		std::vector<AnimationInfo> animationBuffer;
		std::vector<SkinInfo> skinBuffer;
	};

}
