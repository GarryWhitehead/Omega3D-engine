#include "TransformManager.h"

#include "Core/Omega_Common.h"
#include "Core/Omega_Global.h"

#include "Managers/EventManager.h"
#include "Managers/MeshManager.h"
#include "Managers/ObjectManager.h"

#include "Models/ModelSkin.h"
#include "Models/ModelTransform.h"

#include "Types/ComponentTypes.h"

#include "Utility/GeneralUtil.h"
#include "VulkanAPI/Managers/BufferManager.h"

#include <algorithm>
#include <cstdint>

namespace OmegaEngine
{

TransformManager::TransformManager()
{
	transformAligned = VulkanUtil::alignmentSize(sizeof(TransformBufferInfo));
	skinnedAligned = VulkanUtil::alignmentSize(sizeof(SkinnedBufferInfo));

	// allocate the memory used to store the transforms on the CPU side. This will be aligned as we are using dynamic buffers on the Vulkan side
	transformBufferData =
	    (TransformBufferInfo*)Util::alloc_align(transformAligned, transformAligned * TransformBlockSize);
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

bool TransformManager::addNodeHierachy(ModelNode& node, Object& obj, ModelSkin* skin, size_t count)
{
	if (!node.rootNode)
	{
		LOGGER_ERROR("Trying to add a root node that is null.\n");
		return false;
	}

	// add skins to the manager - these don't require a slot to be requested as there
	// may be numerous skins per mesh. Instead, the starting index of this group
	// will be used to offset the skin indices to point at the correct skin
	size_t skinIdx = skins.size();

	for (size_t i = 0; i < count; ++i)
	{
		skins.emplace_back(skin[i]);
	}

	TransformInfo info{ std::move(node.rootNode), skinIdx };

	// request a slot for this object
	size_t idx = addObject(obj);

	if (idx > nodes.size())
	{
		// We must take ownership here to stop dangling pointers.
		// We have no idea when the model data will go out of scope
		nodes.emplace_back(std::move(info));
	}
}

void TransformManager::addTransform(OEMaths::mat4f& local, OEMaths::vec3f& translation, OEMaths::vec3f& scale,
                                    OEMaths::quatf& rot)
{
	ModelNode::NodeInfo* newNode = new ModelNode::NodeInfo();
	newNode->translation = translation;
	newNode->scale = scale;
	newNode->rotation = rot;
	newNode->nodeTransform = local;
	newNode->localTransform = local;

	nodes.push_back({ newNode, 0 });
}

OEMaths::mat4f TransformManager::updateNodeLocalMatrix(ModelNode::NodeInfo* node, OEMaths::mat4f& world)
{
	OEMaths::mat4f mat = node->nodeTransform;
	ModelNode::NodeInfo* parent = node->parent;
	while (parent)
	{
		mat = parent->nodeTransform * mat;
		parent = parent->parent;
	}

	return mat * world;
}


void TransformManager::updateTransformRecursive(Object& obj,
                                                uint32_t transformAlignment, uint32_t skinnedAlignment)
{
	OEMaths::mat4f mat;

	// an object with a mesh component should always contain a transform, but better safe than sorry.
	if (obj.hasComponent<MeshComponent>() && obj.hasComponent<TransformComponent>())
	{
		auto& transformComponent = obj.getComponent<TransformComponent>();
		uint32_t objIndex = transformComponent.index;

		TransformBufferInfo* transformBuffer =
		    (TransformBufferInfo*)((uint64_t)transformBufferData + (transformAlignment * transformBufferSize));

		transformComponent.dynamicUboOffset = transformBufferSize * transformAlignment;

		mat = updateMatrixFromTree(obj, objectManager);
		transformBuffer->modelMatrix = mat;

		++transformBufferSize;

		// If the object has a skinned element, then deal with that now
		if (obj.hasComponent<SkinnedComponent>())
		{
			auto& skinnedComponent = obj.getComponent<SkinnedComponent>();
			SkinnedBufferInfo* skinnedBufferPtr =
			    (SkinnedBufferInfo*)((uint64_t)skinnedBufferData + (skinnedAlignment * skinnedBufferSize));

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
				Object* joint_obj = skinBuffer[skinIndex].joints[i];
				OEMaths::mat4f jointMatrix =
				    updateMatrixFromTree(*joint_obj, objectManager) * skinBuffer[skinIndex].invBindMatrices[i];

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

	for (auto& child : children)
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
			VulkanAPI::BufferUpdateEvent event{ "Transform", (void*)transformBufferData,
				                                transformAligned * transformBufferSize,
				                                VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
		}
		if (skinnedBufferSize)
		{
			VulkanAPI::BufferUpdateEvent event{ "SkinnedTransform", (void*)skinnedBufferData,
				                                skinnedAligned * skinnedBufferSize,
				                                VulkanAPI::MemoryUsage::VK_BUFFER_DYNAMIC };
			Global::eventManager()->addQueueEvent<VulkanAPI::BufferUpdateEvent>(event);
		}

		isDirty = false;
	}
}

void TransformManager::updateObjectTranslation(Object* obj, OEMaths::vec4f trans)
{
	uint32_t index = obj->getComponent<TransformComponent>().index;
	transforms[index].setTranslation(OEMaths::vec3f{ trans.getX(), trans.getY(), trans.getZ() });

	// this will update all lists - though TODO: add objects which need updating for that frame to a list - should be faster?
	isDirty = true;
}

void TransformManager::updateObjectScale(Object* obj, OEMaths::vec4f scale)
{
	uint32_t index = obj->getComponent<TransformComponent>().index;
	transforms[index].setScale(OEMaths::vec3f{ scale.getX(), scale.getY(), scale.getZ() });
	isDirty = true;
}

void TransformManager::updateObjectRotation(Object* obj, OEMaths::quatf rot)
{
	uint32_t index = obj->getComponent<TransformComponent>().index;
	transforms[index].setRotation(rot);
	isDirty = true;
}
}    // namespace OmegaEngine
