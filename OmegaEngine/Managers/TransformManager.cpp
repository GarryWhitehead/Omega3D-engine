#include "TransformManager.h"
#include "Engine/Omega_Global.h"
#include "Managers/EventManager.h"
#include "Managers/MeshManager.h"
#include "Models/ModelSkin.h"
#include "Models/ModelTransform.h"
#include "ObjectInterface/ComponentTypes.h"
#include "ObjectInterface/Object.h"
#include "ObjectInterface/ObjectManager.h"
#include "Omega_Common.h"
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

std::unique_ptr<ModelTransform> TransformManager::transform(const OEMaths::vec3f &trans,
                                                             const OEMaths::vec3f &sca,
                                                             const OEMaths::quatf &rot)
{
	auto& t = std::make_unique<ModelTransform>(trans, sca, rot);
	return std::move(t);
}

void TransformManager::addComponentToManager(TransformComponent *component)
{
	TransformData transform;

	if (component->transform->hasMatrix)
	{
		transform.setLocalMatrix(component->transform->trsMatrix);
	}
	else
	{
		transform.setTranslation(component->transform->translation);
		transform.setScale(component->transform->scale);
		transform.setRotation(component->transform->rotation);
	}

	transforms.emplace_back(transform);

	component->index = static_cast<uint32_t>(transforms.size() - 1);
}

bool TransformManager::addComponentToManager(SkeletonComponent *component, Object *object)
{
	if (skinBuffer.empty())
	{
		return false;
	}

	uint32_t bufferIndex = component->index + component->bufferOffset;
	assert(bufferIndex < skinBuffer.size());

	// check whether the skeleton root
	if (component->isRoot)
	{
		skinBuffer[bufferIndex].skeleton = object;
	}

	// link skinning info with objects
	skinBuffer[bufferIndex].joints.emplace_back(object);

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

	return mat * world;
}

void TransformManager::updateTransformRecursive(std::unique_ptr<ObjectManager> &objectManager,
                                                Object &obj, uint32_t transformAlignment,
                                                uint32_t skinnedAlignment)
{
	OEMaths::mat4f mat;

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

		if (obj.hasComponent<SkinnedComponent>())
		{
			auto &skinnedComponent = obj.getComponent<SkinnedComponent>();
			SkinnedBufferInfo *skinnedBufferPtr =
			    (SkinnedBufferInfo *)((uint64_t)skinnedBufferData +
			                          (skinnedAlignment * skinnedBufferSize));

			// skinned transform
			skinnedComponent.dynamicUboOffset = skinnedBufferSize * skinnedAlignment;

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

void TransformManager::updateTransform(std::unique_ptr<ObjectManager> &objectManager)
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

void TransformManager::updateFrame(double time, double dt,
                                   std::unique_ptr<ObjectManager> &objectManager,
                                   ComponentInterface *componentInterface)
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
