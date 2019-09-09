#include "TransformManager.h"

#include "Core/Omega_Global.h"
#include "Core/Omega_Common.h"

#include "Managers/EventManager.h"
#include "Managers/MeshManager.h"
#include "Managers/ObjectManager.h"

#include "Models/ModelSkin.h"
#include "Models/ModelTransform.h"

#include "Types/ComponentTypes.h"

#include "Utility/GeneralUtil.h"
#include "VulkanAPI/BufferManager.h"

#include <algorithm>
#include <cstdint>

namespace OmegaEngine
{

TransformManager::TransformManager()
{
	transformAligned = VulkanAPI::Util::alignmentSize(sizeof(TransformBufferInfo));
	skinnedAligned = VulkanAPI::Util::alignmentSize(sizeof(SkinnedBufferInfo));

	// allocate the memory used to store the transforms on the CPU side. This will be aligned as we are using dynamic buffers on the Vulkan side
	transformBufferData = (TransformBufferInfo *)Util::alloc_align(
	    transformAligned, transformAligned * TransformBlockSize);
	skinnedBufferData =
	    (SkinnedBufferInfo *)Util::alloc_align(skinnedAligned, skinnedAligned * SkinnedBlockSize);
}

TransformManager::~TransformManager()
{
	if (transformBufferData)
	{
		_aligned_free(transformBufferData);
	}
	if (skinnedBufferData)
	{
		_aligned_free(skinnedBufferData);
	}
}

void TransformManager::addTransform(std::unique_ptr<ModelTransform>& trans, Object& obj)
{
	TransformData transform;

	size_t index = 0;
	if (trans->hasMatrix)
	{
		transform.setLocalMatrix(trans->trsMatrix);
	}
	else
	{
		transform.setTranslation(trans->translation);
		transform.setScale(trans->scale);
		transform.setRotation(trans->rotation);	
	}

	transforms.emplace_back(transform);

	TransformComponent comp;
	comp.setIndex(index);
	obj.addComponent<TransformComponent>(comp);
}

bool TransformManager::addSkeleton(size_t index, bool isRoot, Object *object)
{
	if (skinBuffer.empty())
	{
		return false;
	}

	assert(index < skinBuffer.size());

	// check whether the skeleton root
	if (isRoot)
	{
		skinBuffer[index].skeleton = object;
	}

	// link skinning info with objects
	skinBuffer[index].joints.emplace_back(object);

	return true;
}

void TransformManager::addSkin(std::unique_ptr<ModelSkin> &skin)
{
	SkinInfo skinInfo;

	// inv bind matrices are just a straight copy
	skinInfo.jointMatrices.resize(skin->jointMatrices.size());
	memcpy(skinInfo.jointMatrices.data(), skin->jointMatrices.data(),
	       skinInfo.jointMatrices.size() * sizeof(OEMaths::mat4f));

	skinInfo.invBindMatrices.resize(skin->invBindMatrices.size());
	memcpy(skinInfo.invBindMatrices.data(), skin->invBindMatrices.data(),
	       skinInfo.invBindMatrices.size() * sizeof(OEMaths::mat4f));

	// rest of the skin info will be added later
	skinBuffer.emplace_back(skinInfo);
}

OEMaths::mat4f TransformManager::updateMatrixFromTree(Object &obj,
                                                      std::unique_ptr<ObjectManager> &objectManager)
{
	uint32_t objIndex = obj.getComponent<TransformComponent>().index;
	OEMaths::mat4f mat = transforms[objIndex].getLocalMatrix();

	Object* parentObject = &obj;
	uint64_t parentId = parentObject->getParent();
	
	// 0xfffff signifies that this is the root transform
	while (parentId != UINT64_MAX)
	{
		parentObject = objectManager->getObject(parentId);

		if (parentObject->hasComponent<TransformComponent>())
		{
			uint32_t parentIndex = parentObject->getComponent<TransformComponent>().index;
			mat = transforms[parentIndex].getLocalMatrix() * mat;
		}
		parentId = parentObject->getParent();
	}

	// the root object should contain the world transform - though make sure
	OEMaths::mat4f world;
	if (parentObject->hasComponent<WorldTransformComponent>())
	{
		auto component = parentObject->getComponent<WorldTransformComponent>();
		OEMaths::mat4f rot = OEMaths::mat4f(component.rotation);
		world = OEMaths::mat4f::translate(component.translation) * rot *
		        OEMaths::mat4f::scale(component.scale);
	}

	// all local matrices are transformed by the world matrix
	return mat * world;
}

void TransformManager::updateTransformRecursive(std::unique_ptr<ObjectManager> &objectManager,
                                                Object &obj, uint32_t transformAlignment,
                                                uint32_t skinnedAlignment)
{
	OEMaths::mat4f mat;

	// an object with a mesh component should always contain a transform, but better safe than sorry.
	if (obj.hasComponent<MeshComponent>() && obj.hasComponent<TransformComponent>())
	{
		auto &transformComponent = obj.getComponent<TransformComponent>();
		uint32_t objIndex = transformComponent.index;

		TransformBufferInfo *transformBuffer =
		    (TransformBufferInfo *)((uint64_t)transformBufferData +
		                            (transformAlignment * transformBufferSize));

		transformComponent.dynamicUboOffset = transformBufferSize * transformAlignment;

		mat = updateMatrixFromTree(obj, objectManager);
		transformBuffer->modelMatrix = mat;

		++transformBufferSize;

		// If the object has a skinned element, then deal with that now
		if (obj.hasComponent<SkinnedComponent>())
		{
			auto &skinnedComponent = obj.getComponent<SkinnedComponent>();
			SkinnedBufferInfo *skinnedBufferPtr =
			    (SkinnedBufferInfo *)((uint64_t)skinnedBufferData +
			                          (skinnedAlignment * skinnedBufferSize));

			// the dynamic offset index is stored on the transfrom component
			skinnedComponent.dynamicUboOffset = skinnedBufferSize * skinnedAlignment;

			// the transform data index for this object is stored on the component
			uint32_t skinIndex = skinnedComponent.index;

			// prepare fianl output matrices buffer
			uint64_t jointSize = static_cast<uint32_t>(skinBuffer[skinIndex].joints.size()) > 256 ?
			                         256 :
			                         skinBuffer[skinIndex].joints.size();
			skinBuffer[skinIndex].jointMatrices.resize(jointSize);

			skinnedBufferPtr->jointCount = jointSize;

			// transform to local space
			OEMaths::mat4f inverseMat = mat.inverse();

			for (uint32_t i = 0; i < jointSize; ++i)
			{
				Object *joint_obj = skinBuffer[skinIndex].joints[i];
				OEMaths::mat4f jointMatrix = updateMatrixFromTree(*joint_obj, objectManager) *
				                             skinBuffer[skinIndex].invBindMatrices[i];

				// transform joint to local (joint) space
				OEMaths::mat4f localMatrix = inverseMat * jointMatrix;
				skinBuffer[skinIndex].jointMatrices[i] = localMatrix;
				skinnedBufferPtr->jointMatrices[i] = localMatrix;
			}

			++skinnedBufferSize;
		}
	}

	// now update all child nodes too - TODO: do this without recursion
	auto children = obj.getChildren();

	for (auto &child : children)
	{
		updateTransformRecursive(objectManager, child, transformAlignment, skinnedAlignment);
	}
}

void TransformManager::updateTransform(ObjectManager& objectManager)
{
	// TODO: refactor this
	auto objectList = objectManager->getObjectsList();

	transformBufferSize = 0;
	skinnedBufferSize = 0;

	for (auto obj : objectList)
	{
		updateTransformRecursive(objectManager, obj.second, transformAligned, skinnedAligned);
	}
}

void TransformManager::updateFrame(ObjectManager& objectManager)
{
	// check whether static data need updating
	if (isDirty)
	{
		updateTransform(objectManager);

		if (transformBufferSize)
		{
			VulkanAPI::BufferUpdateEvent event{ "Transform", (void *)transformBufferData,
				                                transformAligned * transformBufferSize,
				                                VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
		}
		if (skinnedBufferSize)
		{
			VulkanAPI::BufferUpdateEvent event{ "SkinnedTransform", (void *)skinnedBufferData,
				                                skinnedAligned * skinnedBufferSize,
				                                VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
		}

		isDirty = false;
	}
}

void TransformManager::updateObjectTranslation(Object *obj, OEMaths::vec4f trans)
{
	uint32_t index = obj->getComponent<TransformComponent>().index;
	transforms[index].setTranslation(OEMaths::vec3f{ trans.getX(), trans.getY(), trans.getZ() });

	// this will update all lists - though TODO: add objects which need updating for that frame to a list - should be faster?
	isDirty = true;
}

void TransformManager::updateObjectScale(Object *obj, OEMaths::vec4f scale)
{
	uint32_t index = obj->getComponent<TransformComponent>().index;
	transforms[index].setScale(OEMaths::vec3f{ scale.getX(), scale.getY(), scale.getZ() });
	isDirty = true;
}

void TransformManager::updateObjectRotation(Object *obj, OEMaths::quatf rot)
{
	uint32_t index = obj->getComponent<TransformComponent>().index;
	transforms[index].setRotation(rot);
	isDirty = true;
}
} // namespace OmegaEngine
