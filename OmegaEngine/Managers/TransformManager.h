#pragma once
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_transform.h"
#include "OEMaths/OEMaths_Quat.h"
#include "Managers/ManagerBase.h"
#include "Objects/Object.h"
#include "Utility/logger.h"
#include "Vulkan/MemoryAllocator.h"

#include "tiny_gltf.h"

#include <vector>
#include <cstdint>

namespace OmegaEngine
{
	// forward decleartions
	class ObjectManager;

	class TransformManager : public ManagerBase
	{

	public:

		struct TransformData
		{
			// static 
			struct LocalTransform
			{
				OEMaths::vec3f trans;
				OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f, 1.0f, 1.0f };
				OEMaths::mat4f rotation;
			};

			void calculateLocalMatrix()
			{
				local = OEMaths::mat4f::translate(localTransform.trans) * localTransform.rotation * OEMaths::mat4f::scale(localTransform.scale);
			}

			uint32_t getTransformOffset() const
			{
				return transformBufferOffset;
			}

			uint32_t getSkinnedOffset() const
			{
				return skinnedBufferOffset;
			}

			void setTransformOffset(const uint32_t offset)
			{
				transformBufferOffset = offset;
			}

			void setSkinnedOffset(const uint32_t offset)
			{
				skinnedBufferOffset = offset;
			}

			void setTranslation(OEMaths::vec3f& trans)
			{
				localTransform.trans = trans;
				recalculateLocal = true;
			}

			void setRotation(OEMaths::mat4f& rot)
			{
				localTransform.rotation = rot;
				recalculateLocal = true;
			}

			void setRotation(OEMaths::quatf& q)
			{
				localTransform.rotation = OEMaths::mat4f(q);
				recalculateLocal = true;
			}

			void setScale(OEMaths::vec3f& scale)
			{
				localTransform.scale = scale;
				recalculateLocal = true;
			}

			OEMaths::mat4f getLocalMatrix()
			{
				if (recalculateLocal) 
				{
					calculateLocalMatrix();
					recalculateLocal = false;
				}
				return local;
			}

			void setLocalMatrix(OEMaths::mat4f& local)
			{
				this->local = local;
			}

			void setWorldMatixrix(OEMaths::mat4f& world)
			{
				this->world = world;
			}

			void setSkinIndex(const int32_t index)
			{
				skinIndex = index;
			}

			int32_t getSkinIndex() const
			{
				return skinIndex;
			}

		private:

			bool recalculateLocal = false;

			// decomposed form
			LocalTransform localTransform;

			OEMaths::mat4f local;
			OEMaths::mat4f world;

			// an index to skinning data for this particular node - negative number indicates no skin info
			int32_t skinIndex = -1;

			// buffer offsets for tranform and skinned data
			uint32_t transformBufferOffset = 0;
			uint32_t skinnedBufferOffset = 0;
		};

		// data that will be hosted on the gpu side
		struct TransformBufferInfo
		{
			OEMaths::mat4f modelMatrix;
		};

		struct SkinnedBufferInfo
		{
			OEMaths::mat4f jointMatrices[6];
			float jointCount;
		};

		// skinning data derived from file 
		struct SkinInfo
		{
			const char* name;
			Object skeletonIndex;

			std::vector<Object> joints;
			std::vector<OEMaths::mat4f> invBindMatrices;
			std::vector<OEMaths::mat4f> jointMatrices;
		};

		// the number of models to allocate mem space for - this will need optimising
		// could also be dynamic and be altered to the archietecture being used
		const uint32_t TransformBlockSize = 25;
		const uint32_t SkinnedBlockSize = 25;

		TransformManager();
		~TransformManager();

		// update per frame 
		void updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface) override;

		// gltf loading - The skinning data is going in the transform manager for now as it's needed most here for calculating skinning transforms
		void addGltfSkin(tinygltf::Model& model, std::unordered_map<uint32_t, Object>& linearisedObjects);

		// local transform and skinning update
		OEMaths::mat4f updateMatrixFromTree(Object& obj, std::unique_ptr<ObjectManager>& objectManager);
		void updateTransform(std::unique_ptr<ObjectManager>& objectManager);
		void updateTransformRecursive(std::unique_ptr<ObjectManager>& objectManager, Object& obj, uint32_t alignment, uint32_t skinnedAlignment);

		// object update functions
		void updateObjectTranslation(Object& obj, OEMaths::vec4f trans);
		void updateObjectScale(Object& obj, OEMaths::vec4f scale);
		void updateObjectRotation(Object& obj, OEMaths::quatf rot);

		uint32_t getTransformOffset(uint32_t id)
		{
			// the transform buffer stores both parents and children so unfortunately we need to iterate through
			// to find the id.
			if (transforms.find(id) == transforms.end()) 
			{
				LOGGER_ERROR("Unable to find object data with an id of %i within transform manager.", id);
			}
			return transforms[id].getTransformOffset();
		}

		uint32_t getSkinnedOffset(uint32_t id)
		{
			// the transform buffer stores both parents and children so unfortunately we need to iterate through
			// to find the id.
			if (transforms.find(id) == transforms.end()) 
			{
				LOGGER_ERROR("Unable to find object data with an id of %i within transform manager.", id);
			}
			return transforms[id].getSkinnedOffset();
		}

	private:

		// transform data for static meshes - stored with object id for faster look up
		std::unordered_map<uint32_t, TransformData> transforms;

		// skinned transform data
		std::vector<SkinInfo> skinBuffer;

		// store locally the aligned buffer sizes
		uint32_t transformAligned = 0;
		uint32_t skinnedAligned = 0;

		// transform data for each object which will be added to the GPU
		TransformBufferInfo* transformBufferData = nullptr;
		SkinnedBufferInfo* skinnedBufferData = nullptr;

		uint32_t transformBufferSize = 0;
		uint32_t skinnedBufferSize = 0;

		// flag which tells us whether we need to update the static data
		bool isDirty = true;
	};

}

