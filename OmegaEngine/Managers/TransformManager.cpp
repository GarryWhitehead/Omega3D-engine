#include "TransformManager.h"
#include "Objects/Object.h"
#include "Objects/ObjectManager.h"

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

	void TransformManager::addGltfTransform(tinygltf::Node& node, Object* obj, OEMaths::mat4f worldTransform)
	{
		TransformData transform;

		// we will save the matrix and the decomposed form
		if (node.translation.size() == 3) 
		{
			transform.setTranslation(OEMaths::vec3f{ (float)node.translation[0], (float)node.translation[1], (float)node.translation[2] });
		}
		if (node.scale.size() == 3) 
		{
			transform.setScale(OEMaths::vec3f{ (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] });
		}
		if (node.rotation.size() == 4) 
		{
			OEMaths::quatf quat(node.rotation.data());
			transform.setRotation(quat);
		}

		// world transform is obtained from the omega scene file
		transform.setWorldMatixrix(worldTransform);

		if (node.matrix.size() == 16) 
		{
			transform.setLocalMatrix(OEMaths::mat4f(node.matrix.data()));
		}
		
		// also add index to skinning information if applicable
		transform.setSkinIndex(node.skin);

		transformBuffer[obj->getId()] = transform;

		// add to the list of entites
		obj->addComponent<TransformComponent>(static_cast<uint32_t>(transformBuffer.size() - 1));
	}

	void TransformManager::addGltfSkin(tinygltf::Model& model, std::unordered_map<uint32_t, Object>& linearisedObjects)
	{
		for (tinygltf::Skin& skin : model.skins) 
		{
			SkinInfo skinInfo;
			skinInfo.name = skin.name.c_str();

			// Is this the skeleton root node?
			if (skin.skeleton > -1) 
			{
				assert(skin.skeleton < linearisedObjects.size());
				skinInfo.skeletonIndex = linearisedObjects[skin.skeleton];
				skinInfo.skeletonIndex.addComponent<SkinnedComponent>(static_cast<uint32_t>(skinBuffer.size() - 1));
			}

			// Does this skin have joint nodes?
			for (auto& jointIndex : skin.joints) 
			{
				// we will check later if this node actually exsists
				assert(jointIndex < linearisedObjects.size() && jointIndex > -1);
				skinInfo.joints.push_back(linearisedObjects[jointIndex]);
			}

			// get the inverse bind matricies, if there are any
			if (skin.inverseBindMatrices > -1) 
			{
				tinygltf::Accessor accessor = model.accessors[skin.inverseBindMatrices];
				tinygltf::BufferView bufferView = model.bufferViews[accessor.bufferView];
				tinygltf::Buffer buffer = model.buffers[bufferView.buffer];

				skinInfo.invBindMatrices.resize(accessor.count);
				memcpy(skinInfo.invBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(OEMaths::mat4f));
			}

			skinBuffer.push_back(skinInfo);
		}
	}

	OEMaths::mat4f TransformManager::updateMatrixFromTree(Object& obj, std::unique_ptr<ObjectManager>& objectManager)
	{
		OEMaths::mat4f mat = transformBuffer[obj.getId()].getLocalMatrix();

		uint64_t parentId = obj.getParent();
		while (parentId != UINT64_MAX) 
		{
			Object* parentObject = objectManager->getObject(parentId);

			uint32_t id = parentObject->getId();
			mat = transformBuffer[id].getLocalMatrix() * mat;
			parentId = parentObject->getParent();
		}

		return mat;
	}

	void TransformManager::updateTransformRecursive(std::unique_ptr<ObjectManager>& objectManager, Object& obj, uint32_t transformAlignment, uint32_t skinnedAlignment)
	{
		if (obj.hasComponent<MeshComponent>() && obj.hasComponent<SkinnedComponent>()) 
		{
			uint32_t id = obj.getId();

			TransformBufferInfo* transformBuffer = (TransformBufferInfo*)((uint64_t)transformBufferData + (transformAlignment * transformBufferer_size));
			SkinnedBufferInfo* skinnedBuffer = (SkinnedBufferInfo*)((uint64_t)skinnedBufferData + (skinnedAlignment * skinnedBufferSize));
			
			transformBuffer[id].setTransformOffset(transformBufferSize* transformAlignment);

			OEMaths::mat4f mat = updateMatrixFromTree(obj, objectManager);
			transformBuffer->modelMatrix = mat * OEMaths::mat4f::scale(OEMaths::vec3f{ 3.0f, 3.0f, 3.0f });
			
			++transformBufferSize;

			// skinned transform
			transformBuffer[id].setSkinnedOffset(skinnedBufferSize * skinnedAlignment);

			uint32_t skinIndex = transformBuffer[id].getSkinIndex();

			// prepare fianl output matrices buffer
			uint64_t jointSize = static_cast<uint32_t>(skinBuffer[skinIndex].joints.size()) > 256 ? 256 : skinBuffer[skinIndex].joints.size();
			skinBuffer[skinIndex].jointMatrices.resize(jointSize);

			skinnedBuffer->jointCount = jointSize;

			// transform to local space
			OEMaths::mat4f inverseMat = mat.inverse();

			for (uint32_t i = 0; i < jointSize; ++i) 
			{	
				Object joint_obj = skinBuffer[skinIndex].joints[i];
				OEMaths::mat4f jointMatrix = updateMatrixFromTree(joint_obj, objectManager) * skinBuffer[skinIndex].invBindMatrices[i];

				// transform joint to local (joint) space
				OEMaths::mat4f localMatrix = inverseMat * jointMatrix;
				skinBuffer[skinIndex].jointMatrices[i] = localMatrix;
				skinnedBuffer->jointMatrices[i] = localMatrix;
			}
			++skinnedBufferSize;
	
		}
		else if (obj.hasComponent<MeshComponent>())
		{
			uint32_t id = obj.getId();

			TransformBufferInfo* transformBuffer = (TransformBufferInfo*)((uint64_t)transformBufferData + (transformAlignment * transformBufferSize));
			
			transformBuffer[id].setTransformOffset(transformBufferSize* transformAlignment);

			OEMaths::mat4f mat = updateMatrixFromTree(obj, objectManager);
			transformBuffer->modelMatrix = mat * OEMaths::mat4f::scale(OEMaths::vec3f{ 3.0f, 3.0f, 3.0f });
			
			++transformBufferSize;
		}

		// now update all child nodes too - TODO: do this without recursion
		auto children = obj.getChildren();

		for (auto& child : children) 
		{
			// it is possible that the object has no transform, so check this first
			if (child.hasComponent<TransformManager>()) {
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
				VulkanAPI::BufferUpdateEvent event{ "Transform", (void*)transformBufferData, transformAligned * transformBufferer_size, VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
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
		transformBuffer[id].setTranslation(OEMaths::vec3f{ trans.getX(), trans.getY(), trans.getZ() });

		// this will update all lists - though TODO: add objects which need updating for that frame to a list - should be faster?
		isDirty = true;		
	}

	void TransformManager::updateObjectScale(Object& obj, OEMaths::vec4f scale)
	{
		uint32_t id = obj.getId();
		transformBuffer[id].setScale(OEMaths::vec3f{ scale.getX(), scale.getY(), scale.getZ() });
		isDirty = true;
	}

	void TransformManager::updateObjectRotation(Object& obj, OEMaths::quatf rot)
	{
		uint32_t id = obj.getId();
		transformBuffer[id].setRotation(rot);
		isDirty = true;
	}
}
