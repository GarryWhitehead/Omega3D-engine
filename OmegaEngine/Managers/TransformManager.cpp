#include "TransformManager.h"
#include "ObjectInterface/Object.h"
#include "ObjectInterface/ObjectManager.h"
#include "ObjectInterface/Object.h"
#include "ObjectInterface/ComponentTypes.h"
#include "Models/ModelTransform.h"
#include "Omega_Common.h"
#include "Vulkan/BufferManager.h"
#include "Managers/EventManager.h"
#include "Managers/MeshManager.h"
#include "Engine/Omega_Global.h"
#include "Utility/GeneralUtil.h"

#include <cstdint>
#include <algorithm>

namespace OmegaEngine
{

	TransformManager::TransformManager()
	{
		transformAligned = VulkanAPI::Util::alignmentSize(sizeof(TransformBufferInfo));
		skinnedAligned = VulkanAPI::Util::alignmentSize(sizeof(SkinnedBufferInfo));

		// allocate the memory used to store the transforms on the CPU side. This will be aligned as we are using dynamic buffers on the Vulkan side
		transformBufferData = (TransformBufferInfo*)Util::alloc_align(transformAligned, transformAligned * TransformBlockSize);
		skinnedBufferData = (SkinnedBufferInfo*)Util::alloc_align(skinnedAligned, skinnedAligned * SkinnedBlockSize);
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

	void TransformManager::addComponentToManager(TransformComponent& component, Object& object)
	{
		TransformData transform;

		if (component.transform->hasTrsMatrix())
		{
			transform.setLocalMatrix(component.transform->getMatrix());
		}
		else
		{
			transform.setTranslation(component.transform->getTranslation());
			transform.setScale(component.transform->getScale());
			transform.setRotation(component.transform->getRotation());
		}

		transforms.emplace(object.getId(), transform);
	}

	OEMaths::mat4f TransformManager::updateMatrixFromTree(Object& obj, std::unique_ptr<ObjectManager>& objectManager)
	{
		OEMaths::mat4f mat = transforms[obj.getId()].getLocalMatrix();

		uint64_t parentId = obj.getParent();
		while (parentId != UINT64_MAX) 
		{
			Object* parentObject = objectManager->getObject(parentId);

			uint32_t id = parentObject->getId();
			mat = transforms[id].getLocalMatrix() * mat;
			parentId = parentObject->getParent();
		}

		return mat;
	}

	void TransformManager::updateTransformRecursive(std::unique_ptr<ObjectManager>& objectManager, Object& obj, uint32_t transformAlignment, uint32_t skinnedAlignment)
	{
		if (obj.hasComponent<MeshComponent>() && obj.hasComponent<SkinnedComponent>()) 
		{
			uint32_t id = obj.getId();

			TransformBufferInfo* transformBufferPtr = (TransformBufferInfo*)((uint64_t)transformBufferData + (transformAlignment * transformBufferSize));
			SkinnedBufferInfo* skinnedBufferPtr = (SkinnedBufferInfo*)((uint64_t)skinnedBufferData + (skinnedAlignment * skinnedBufferSize));
			
			transforms[id].setTransformOffset(transformBufferSize * transformAlignment);

			OEMaths::mat4f mat = updateMatrixFromTree(obj, objectManager);
			transformBufferPtr->modelMatrix = mat * OEMaths::mat4f::scale(OEMaths::vec3f{ 3.0f, 3.0f, 3.0f });
			
			++transformBufferSize;

			// skinned transform
			transforms[id].setSkinnedOffset(skinnedBufferSize * skinnedAlignment);

			uint32_t skinIndex = transforms[id].getSkinIndex();

			// prepare fianl output matrices buffer
			uint64_t jointSize = static_cast<uint32_t>(skinBuffer[skinIndex].joints.size()) > 256 ? 256 : skinBuffer[skinIndex].joints.size();
			skinBuffer[skinIndex].jointMatrices.resize(jointSize);

			skinnedBufferPtr->jointCount = jointSize;

			// transform to local space
			OEMaths::mat4f inverseMat = mat.inverse();

			for (uint32_t i = 0; i < jointSize; ++i) 
			{	
				Object joint_obj = skinBuffer[skinIndex].joints[i];
				OEMaths::mat4f jointMatrix = updateMatrixFromTree(joint_obj, objectManager) * skinBuffer[skinIndex].invBindMatrices[i];

				// transform joint to local (joint) space
				OEMaths::mat4f localMatrix = inverseMat * jointMatrix;
				skinBuffer[skinIndex].jointMatrices[i] = localMatrix;
				skinnedBufferPtr->jointMatrices[i] = localMatrix;
			}

			++skinnedBufferSize;
		}
		else if (obj.hasComponent<MeshComponent>())
		{
			uint32_t id = obj.getId();

			TransformBufferInfo* transformBuffer = (TransformBufferInfo*)((uint64_t)transformBufferData + (transformAlignment * transformBufferSize));
			
			transforms[id].setTransformOffset(transformBufferSize * transformAlignment);

			OEMaths::mat4f mat = updateMatrixFromTree(obj, objectManager);
			transformBuffer->modelMatrix = mat * OEMaths::mat4f::scale(OEMaths::vec3f{ 3.0f, 3.0f, 3.0f });
			
			++transformBufferSize;
		}

		// now update all child nodes too - TODO: do this without recursion
		auto children = obj.getChildren();

		for (auto& child : children) 
		{
			// it is possible that the object has no transform, so check this first
			if (child.hasComponent<TransformManager>()) 
			{
				updateTransformRecursive(objectManager, child, transformAlignment, skinnedAlignment);
			}
		}
	}

	void TransformManager::updateTransform(std::unique_ptr<ObjectManager>& objectManager)
	{
		auto objectList = objectManager->getObjectsList();

		transformBufferSize= 0;
		skinnedBufferSize = 0;

		for (auto obj : objectList)
		 {
			updateTransformRecursive(objectManager, obj.second, transformAligned, skinnedAligned);
		}
	}

	void TransformManager::updateFrame(double time, double dt, std::unique_ptr<ObjectManager>& objectManager, ComponentInterface* componentInterface)
	{
		// check whether static data need updating
		if (isDirty) 
		{
			updateTransform(objectManager);

			if (transformBufferSize) 
			{
				VulkanAPI::BufferUpdateEvent event{ "Transform", (void*)transformBufferData, transformAligned * transformBufferSize, VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
				Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
			}
			if (skinnedBufferSize) 
			{
				VulkanAPI::BufferUpdateEvent event{ "SkinnedTransform", (void*)skinnedBufferData, skinnedAligned * skinnedBufferSize, VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
				Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
			}

			isDirty = false;
		}
	}
	
	void TransformManager::updateObjectTranslation(Object& obj, OEMaths::vec4f trans)
	{
		uint32_t id = obj.getId();
		transforms[id].setTranslation(OEMaths::vec3f{ trans.getX(), trans.getY(), trans.getZ() });

		// this will update all lists - though TODO: add objects which need updating for that frame to a list - should be faster?
		isDirty = true;		
	}

	void TransformManager::updateObjectScale(Object& obj, OEMaths::vec4f scale)
	{
		uint32_t id = obj.getId();
		transforms[id].setScale(OEMaths::vec3f{ scale.getX(), scale.getY(), scale.getZ() });
		isDirty = true;
	}

	void TransformManager::updateObjectRotation(Object& obj, OEMaths::quatf rot)
	{
		uint32_t id = obj.getId();
		transforms[id].setRotation(rot);
		isDirty = true;
	}
}
