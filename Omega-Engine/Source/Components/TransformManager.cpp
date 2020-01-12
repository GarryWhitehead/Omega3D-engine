#include "TransformManager.h"

#include "Components/RenderableManager.h"

#include "Core/ObjectManager.h"

#include "ModelImporter/SkinInstance.h"
#include "ModelImporter/NodeInstance.h"

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

bool TransformManager::addNodeHierachy(NodeInstance& node, OEObject& obj, SkinInstance* skin)
{
	if (!node.getRootNode())
	{
		LOGGER_ERROR("Trying to add a root node that is null.\n");
		return false;
	}

	// add skins to the manager - these don't require a slot to be requested as there
	// may be numerous skins per mesh. Instead, the starting index of this group
	// will be used to offset the skin indices to point at the correct skin
	size_t skinIdx = skins.size();
    skins.emplace_back(*skin);

    TransformInfo info;
    info.root = std::move(node.getRootNode());
    info.skinOffset = skinIdx;

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
    
    return true;
}

void TransformManager::addTransform(OEMaths::mat4f& local, OEMaths::vec3f& translation, OEMaths::vec3f& scale, OEMaths::quatf& rot)
{
    TransformInfo info;
    info.root = new NodeInfo();
	info.root->translation = translation;
	info.root->scale = scale;
	info.root->rotation = rot;
	info.root->nodeTransform = local;
	info.root->localTransform = local;

	nodes.emplace_back(info);
}

OEMaths::mat4f TransformManager::updateMatrix(NodeInfo* node)
{
	OEMaths::mat4f mat = node->nodeTransform;
	NodeInfo* parent = node->parent;
	while (parent)
	{
		mat = parent->nodeTransform * mat;
		parent = parent->parent;
	}
    return mat;
}

void TransformManager::updateModelTransform(NodeInfo* parent, TransformInfo& transInfo)
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
		if (transInfo.skinOffset != UINT32_MAX)
		{
			// the transform data index for this object is stored on the component
			uint32_t skinIndex = parent->skinIndex;
            assert(skinIndex >= 0);
            SkinInstance& skin = skins[skinIndex];

			// get the number of joints in the skeleton
			uint32_t jointCount = std::min(static_cast<uint32_t>(skin.jointNodes.size()), MAX_BONE_COUNT);

			// transform to local space
			OEMaths::mat4f inverseMat = OEMaths::mat4f::inverse(mat);

			for (uint32_t i = 0; i < jointCount; ++i)
			{
				// get a pointer to the joint a.k.a transform node
				NodeInfo* jointNode = skin.jointNodes[i];

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
	for (NodeInfo* child : parent->children)
	{
		updateModelTransform(child, transInfo);
	}
}

void TransformManager::updateModel(OEObject& obj)
{
	size_t idx = getObjIndex(obj);
	TransformInfo& info = nodes[idx];
	updateModelTransform(info.root->parent, info);
}

void TransformManager::updateObjectTranslation(OEObject& obj, const OEMaths::vec3f& trans)
{
	size_t idx = getObjIndex(obj);
	nodes[idx].root->translation = OEMaths::vec3f{ trans.x, trans.y, trans.z };
}

void TransformManager::updateObjectScale(OEObject& obj, const OEMaths::vec3f& scale)
{
	size_t idx = getObjIndex(obj);
	nodes[idx].root->scale = OEMaths::vec3f{ scale.x, scale.y, scale.z };
}

void TransformManager::updateObjectRotation(OEObject& obj, const OEMaths::quatf& rot)
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
