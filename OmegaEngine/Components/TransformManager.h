#pragma once
#include "Types/Object.h"

#include "OEMaths/OEMaths.h"
#include "OEMaths/OEMaths_Quat.h"
#include "OEMaths/OEMaths_transform.h"

#include "Utility/logger.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace OmegaEngine
{
// forward decleartions
class ObjectManager;
struct TransformComponent;
struct SkinnedComponent;
struct ModelSkin;
class ModelTransform;

class TransformManager
{

public:
	struct TransformData
	{
		/// static
		struct DecompTransform
		{
			OEMaths::vec3f translation;
			OEMaths::vec3f scale = OEMaths::vec3f{ 1.0f };
			OEMaths::mat4f rotation;
		};

		/**
		 * Sets the translation of the decomposed form. 
		 * The local matrix will be re-calculated on calling **getLocalMatrix** 
		 */
		void setTranslation(OEMaths::vec3f& trans)
		{
			decomp.translation = trans;
			recalculateLocal = true;
		}

		/**
		 * Sets the rotation of the decomposed form. 
		 * The local matrix will be re-calculated on calling **getLocalMatrix** 
		 */
		void setRotation(OEMaths::mat4f& rot)
		{
			decomp.rotation = rot;
			recalculateLocal = true;
		}

		/**
		 * Sets the translation of the decomposed form from a quaternoin. 
		 * The local matrix will be re-calculated on calling **getLocalMatrix** 
		 * @param q: rotation as a quaternoin
		 */
		void setRotation(OEMaths::quatf& q)
		{
			decomp.rotation = OEMaths::mat4f(q);
			recalculateLocal = true;
		}

		/**
		 * Sets the scale of the decomposed form. 
		 * The local matrix will be re-calculated on calling **getLocalMatrix** 
		 */
		void setScale(OEMaths::vec3f& scale)
		{
			decomp.scale = scale;
			recalculateLocal = true;
		}

		/**
		 * returns the local transform matrix. If any of the decomposed elements have changed,
		 * the matrix will be re-calculated first.
		 * @return A 4x4 transfrom matrix
		 */
		OEMaths::mat4f getLocalMatrix()
		{
			if (recalculateLocal)
			{
				calculateLocalMatrix();
				recalculateLocal = false;
			}
			return local;
		}

		/**
		 * Sets the local transform matrix.
		 * @param A 4x4 transfrom matrix
		 */
		void setLocalMatrix(OEMaths::mat4f& local)
		{
			this->local = local;
		}

		/**
		 * Sets the world transform matrix.
		 * @param A 4x4 transfrom matrix
		 */
		void setWorldMatixrix(OEMaths::mat4f& world)
		{
			this->world = world;
		}

	private:
		/**
		 *  Calculates the local transform matrix based on the deomposed form
		 */
		void calculateLocalMatrix()
		{
			local = OEMaths::mat4f::translate(localTransform.translation) * localTransform.rotation *
			        OEMaths::mat4f::scale(localTransform.scale);
		}

	private:
		bool recalculateLocal = false;

		/// decomposed form
		DecompTransform decomp;

		OEMaths::mat4f local;
		OEMaths::mat4f world;
	};

	/// data that will be hosted on the gpu side
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
		const char* name;
		Object* skeleton;
		std::vector<Object*> joints;
		std::vector<OEMaths::mat4f> invBindMatrices;
		std::vector<OEMaths::mat4f> jointMatrices;
	};

	// the number of models to allocate mem space for - this will need optimising
	// could also be dynamic and be altered to the archietecture being used
	const uint32_t TransformBlockSize = 25;
	const uint32_t SkinnedBlockSize = 25;

	TransformManager();
	~TransformManager();

	void addTransform(std::unique_ptr<ModelTransform>& trans, Object& obj);
	bool addSkeleton(size_t index, bool isRoot, Object* object);
	void addSkin(std::unique_ptr<ModelSkin>& skin);

	// update per frame
	void updateFrame(ObjectManager& objectManager);

	// local transform and skinning update
	OEMaths::mat4f updateMatrixFromTree(Object& obj, std::unique_ptr<ObjectManager>& objectManager);
	void updateTransform(ObjectManager& objectManager);
	void updateTransformRecursive(std::unique_ptr<ObjectManager>& objectManager, Object& obj, uint32_t alignment,
	                              uint32_t skinnedAlignment);

	// object update functions
	void updateObjectTranslation(Object* obj, OEMaths::vec4f trans);
	void updateObjectScale(Object* obj, OEMaths::vec4f scale);
	void updateObjectRotation(Object* obj, OEMaths::quatf rot);

	size_t getSkinIndex() const
	{
		return skinBuffer.size();
	}

private:

	// indice into the transform buffer for each object stored in this manager
	std::unordered_map<Object, size_t, ObjHash, ObjEqual> objIndices;

	// transform data for static meshes
	std::vector<TransformData> transforms;

	// skinned transform data
	std::vector<SkinInfo> skinBuffer;

	// store locally the aligned buffer sizes
	size_t transformAligned = 0;
	size_t skinnedAligned = 0;

	// transform data for each object which will be added to the GPU
	TransformBufferInfo* transformBufferData = nullptr;
	SkinnedBufferInfo* skinnedBufferData = nullptr;

	size_t transformBufferSize = 0;
	size_t skinnedBufferSize = 0;

	// flag which tells us whether we need to update the static data
	bool isDirty = true;
};

}    // namespace OmegaEngine
