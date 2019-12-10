#include "TransformManager.h"

#include "Components/RenderableManager.h"

#include "Core/ObjectManager.h"

#include "Models/SkinInstance.h"

#include "utility/Logger.h"

#include <algorithm>
#include <cstdint>

namespace OmegaEngine
{

TransformManager::TransformManager()
{
}

TransformManager::~TransformManager()
{
}

bool TransformManager::addNodeHierachy(NodeInstance& node, Object& obj, ModelSkin* skin, size_t count)
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

	// update the model transform, and if skinned, joint matrices
	updateModelTransform(info.root, info);
}

void TransformManager::addTransform(OEMaths::mat4f& local, OEMaths::vec3f& translation, OEMaths::vec3f& scale,
                                    OEMaths::quatf& rot)
{
	NodeInstance::NodeInfo* newNode = new NodeInstance::NodeInfo();
	newNode->translation = translation;
	newNode->scale = scale;
	newNode->rotation = rot;
	newNode->nodeTransform = local;
	newNode->localTransform = local;
	nodes.push_back({ newNode, 0 });
}

OEMaths::mat4f TransformManager::updateMatrix(NodeInstance::NodeInfo* node, OEMaths::mat4f& world)
{
	OEMaths::mat4f mat = node->nodeTransform;
	NodeInstance::NodeInfo* parent = node->parent;
	while (parent)
	{
		mat = parent->nodeTransform * mat;
		parent = parent->parent;
	}

	return mat * world;
}


void TransformManager::updateModelTransform(NodeInstance::NodeInfo* parent, TransformInfo& transInfo)
{
	// we need to find the mesh node first - we will then update matrices working back
	// towards the root node
	if (parent->hasMesh)
	{
		OEMaths::mat4f mat;

		// update the matrices - child node transform * parent transform
		mat = updateMatrix(parent);

		// add updated local transfrom to the ubo buffer
		transInfo.modelTransform = mat;

		// will be null if this model doesn't contain a skin
		if (transInfo.hasSkin)
		{
			// the transform data index for this object is stored on the component
			uint32_t skinIndex = parent->skinIndex;
			ModelSkin& skin = skins[skinIndex];

			// get the number of joints in the skeleton
			size_t jointCount = skin.jointNodes.size();

			// the number of joints is needed on the shader
			transInfo.jointCount = std::min(jointCount, MAX_BONE_COUNT);

			// transform to local space
			OEMaths::mat4f inverseMat = mat.inverse();

			for (uint32_t i = 0; i < jointCount; ++i)
			{
				// get a pointer to the joint a.k.a transform node
				NodeInstance::NodeInfo* jointNode = skin.jointNodes[i];

				// the joint matrix is the local matrix * inverse bin matrix
				OEMaths::mat4f jointMatrix = updateMatrix(jointNode) * skin.invBindMatrices[i];

				// transform joint to local (joint) space
				OEMaths::mat4f localMatrix = inverseMat * jointMatrix;
				transInfo.jointMatrices[i] = localMatrix;
			}
		}

		// one mesh per node is required. So don't bother with the child nodes
		return;
	}

	// now work up the child nodes - until we find a mesh
	for (NodeInstance::NodeInfo* child : parent->children)
	{
		updateModelTransform(child, transInfo);
	}
}

void TransformManager::updateModel(Object& obj)
{
	size_t idx = getObjIndex(obj);
	TransformInfo& info = nodes[idx];
	updateModelTransform(info.root->parent, info);
}

void TransformManager::updateObjectTranslation(Object& obj, const OEMaths::vec3f& trans)
{
	size_t idx = getObjIndex(obj);
	nodes[idx].root->translation = OEMaths::vec3f{ trans.x, trans.y, trans.z };
}

void TransformManager::updateObjectScale(Object& obj, const OEMaths::vec3f& scale)
{
	size_t idx = getObjIndex(obj);
	nodes[idx].root->scale = OEMaths::vec3f{ scale.x, scale.y, scale.z };
}

void TransformManager::updateObjectRotation(Object& obj, const OEMaths::quatf& rot)
{
	size_t idx = getObjIndex(obj);
	nodes[idx].root->rotation = OEMaths::quatf{ rot.x, rot.y, rot.z, rot.w };
}

TransformInfo& TransformManager::getTransform(const ObjHandle handle)
{
	assert(handle > 0 && handle < nodes.size());
	return nodes[handle];
}

}    // namespace OmegaEngine
