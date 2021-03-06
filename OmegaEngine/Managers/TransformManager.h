#pragma once
#include "Managers/ManagerBase.h"
#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "OEMaths/OEMaths_transform.h"
#include "Utility/logger.h"

#include <cstdint>
#include <vector>

namespace OmegaEngine
{
// forward decleartions
class ObjectManager;
class Object;
struct TransformComponent;
struct SkinnedComponent;
struct ModelSkin;

class TransformManager : public ManagerBase
{

public:
	struct TransformData
	{
		// static
		struct LocalTransform
		{
			OEMaths::vec3f translation;
			OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f, 1.0f, 1.0f };
			OEMaths::mat4f rotation;
		};

		void calculateLocalMatrix()
		{
			local = OEMaths::mat4f::translate(localTransform.translation) *
			        localTransform.rotation * OEMaths::mat4f::scale(localTransform.scale);
		}

		void setTranslation(OEMaths::vec3f &trans)
		{
			localTransform.translation = trans;
			recalculateLocal = true;
		}

		void setRotation(OEMaths::mat4f &rot)
		{
			localTransform.rotation = rot;
			recalculateLocal = true;
		}

		void setRotation(OEMaths::quatf &q)
		{
			localTransform.rotation = OEMaths::mat4f(q);
			recalculateLocal = true;
		}

		void setScale(OEMaths::vec3f &scale)
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

		void setLocalMatrix(OEMaths::mat4f &local)
		{
			this->local = local;
		}

		void setWorldMatixrix(OEMaths::mat4f &world)
		{
			this->world = world;
		}

	private:
		bool recalculateLocal = false;

		// decomposed form
		LocalTransform localTransform;

		OEMaths::mat4f local;
		OEMaths::mat4f world;
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

	struct SkinInfo
	{
		const char *name;
		Object *skeleton;
		std::vector<Object *> joints;
		std::vector<OEMaths::mat4f> invBindMatrices;
		std::vector<OEMaths::mat4f> jointMatrices;
	};

	// the number of models to allocate mem space for - this will need optimising
	// could also be dynamic and be altered to the archietecture being used
	const uint32_t TransformBlockSize = 25;
	const uint32_t SkinnedBlockSize = 25;

	TransformManager();
	~TransformManager();

	static std::unique_ptr<ModelTransform> transform(const OEMaths::vec3f &trans,
	                                          const OEMaths::vec3f &sca, const OEMaths::quatf &rot);
	
	void addComponentToManager(TransformComponent *component);
	bool addComponentToManager(SkeletonComponent *component, Object *object);
	void addSkin(std::unique_ptr<ModelSkin> &skin);

	// update per frame
	void updateFrame(double time, double dt, std::unique_ptr<ObjectManager> &objectManager,
	                 ComponentInterface *componentInterface) override;

	// local transform and skinning update
	OEMaths::mat4f updateMatrixFromTree(Object &obj, std::unique_ptr<ObjectManager> &objectManager);
	void updateTransform(std::unique_ptr<ObjectManager> &objectManager);
	void updateTransformRecursive(std::unique_ptr<ObjectManager> &objectManager, Object &obj,
	                              uint32_t alignment, uint32_t skinnedAlignment);

	// object update functions
	void updateObjectTranslation(Object *obj, OEMaths::vec4f trans);
	void updateObjectScale(Object *obj, OEMaths::vec4f scale);
	void updateObjectRotation(Object *obj, OEMaths::quatf rot);

	uint32_t getSkinnedBufferOffset() const
	{
		return static_cast<uint32_t>(skinBuffer.size());
	}

private:
	// transform data for static meshes
	std::vector<TransformData> transforms;

	// skinned transform data
	std::vector<SkinInfo> skinBuffer;

	// store locally the aligned buffer sizes
	uint32_t transformAligned = 0;
	uint32_t skinnedAligned = 0;

	// transform data for each object which will be added to the GPU
	TransformBufferInfo *transformBufferData = nullptr;
	SkinnedBufferInfo *skinnedBufferData = nullptr;

	uint32_t transformBufferSize = 0;
	uint32_t skinnedBufferSize = 0;

	// flag which tells us whether we need to update the static data
	bool isDirty = true;
};

} // namespace OmegaEngine
