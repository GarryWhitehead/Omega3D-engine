#include "BVH.hpp"

#include "OEMaths/OEMaths_transform.h"
#include <queue>
#include <cmath>
#include <set>

namespace OmegaEngine
{

	//Bounding box functions
	BoundingBox::BoundingBox()
	{
		bounds.min = OEMaths::vec3f{ INFINITY, INFINITY, INFINITY };
		bounds.max = OEMaths::vec3f{ -INFINITY, -INFINITY, -INFINITY };
	}

	OEMaths::vec3f BoundingBox::calculateCentroid()
	{
		return { (bounds.min + bounds.max) * 0.5f };
	}

	// Extent functions
	Extents::Extents()
	{
		reset();
	}

	void Extents::reset()
	{
		for (uint8_t c = 0; c < boundingPlaneCount; ++c) {
			distance[c].near = INFINITY;
			distance[c].far = -INFINITY;
		}
	}

	void Extents::extendBounds(const Extents& extents)
	{
		for (uint8_t c = 0; c < boundingPlaneCount; ++c) {
			if (extents.distance[c].near < distance[c].near) {
				distance[c].near = extents.distance[c].near;               // extend plane by the tmin
			}
			if (extents.distance[c].far > distance[c].far) {
				distance[c].far = extents.distance[c].far;                // extend plane by the max value
			}
		}
	}

	bool Extents::testForPlaneIntersect(float& tNear, float& tFar, const float* normDotOrig, const float* normDotDir)
	{
		for (uint8_t c = 0; c < boundingPlaneCount; ++c) {
			float tNearExtents = (distance[c].near - normDotOrig[c]) / normDotDir[c];
			float tFarExtents = (distance[c].far - normDotOrig[c]) / normDotDir[c];

			if (normDotDir[c] < 0.0f) {
				std::swap(tNearExtents, tFarExtents);   // planes have switched so exchange values - stops divide by zero
			}
			if (tNearExtents > tNear) {
				tNear = tNearExtents;                   // confine extents within tnear/tfar
			}
			if (tFarExtents < tFar) {
				tFar = tFarExtents;
			}
			if (tNear > tFar) {
				return false;
			}
		}
		return true;
	}

	OEMaths::vec3f Extents::calculateCentroid()
	{
		return { (distance[0].near + distance[0].far) * 0.5f,           // x-plane
				(distance[1].near + distance[1].far) * 0.5f,            // y-plane
				(distance[2].near + distance[2].far) * 0.5f };          // z-plane
	}

	// Octree functions ===========================================================================================================================

	Octree::Octree(const Extents& sceneExtents)
	{
		// create root bounding box extents based on the min and max values of the canvas
		float xSize = sceneExtents.distance[0].far - sceneExtents.distance[0].near;
		float ySize = sceneExtents.distance[1].far - sceneExtents.distance[1].near;
		float zSize = sceneExtents.distance[2].far - sceneExtents.distance[2].near;
		float maxSize = std::max(xSize, std::max(ySize, zSize));

		OEMaths::vec3f canvasSize{ sceneExtents.distance[0].near + sceneExtents.distance[0].far,
							sceneExtents.distance[1].near + sceneExtents.distance[1].far,
							sceneExtents.distance[2].near + sceneExtents.distance[2].far };

		// set the root bounding box distance
		bbox.bounds.min = (canvasSize - OEMaths::vec3f{ maxSize, maxSize, maxSize }) * 0.5f;
		bbox.bounds.max = (canvasSize + OEMaths::vec3f{ maxSize, maxSize, maxSize }) * 0.5f;

		// creare a new root node
		octree.push_back({ sceneExtents });

		nextFreeIndex = 1;      // there will always be a root node so start from one
	}

	void Octree::buildBvhIterative()
	{
		std::stack<uint32_t> processStack;
		std::stack<uint32_t> bvhStack;

		std::set<uint32_t> visitedNodes;
		uint32_t currentIndex = 0;
		BoundingBox currentBBox;

		// start at the root and find the leaves.....
		processStack.push(currentIndex);

		while (!processStack.empty()) {

			uint32_t currentIndex = processStack.top();
			processStack.pop();

			assert(currentIndex < octree.size());

			if (visitedNodes.count(currentIndex)) {      // doubnle check that we haven't already processed this node - probably not needed as it's highly unlikely to happen
				continue;
			}
			visitedNodes.insert(currentIndex);

			// this stack will be used to generate the bounding volumes from bottom (leaf nodes) to top
			bvhStack.push(currentIndex);

			if (octree[currentIndex].isLeaf) {
				// if this node ia a leaf, extend the extents by each object
				for (auto& extent : octree[currentIndex].nodeExtentsList) {
					octree[currentIndex].nodeExtents.extendBounds(extent);
				}
			}
			else {
				// internal node - adjust the bounding box depending on the child index
				for (uint8_t c = 0; c < childNodeCount; ++c) {
					if (octree[currentIndex].childNode[c] != 0) {
						processStack.push(octree[currentIndex].childNode[c]);
					}
				}
			}
		}

		// now generate bounding volumes starting from the bottom of the tree.....
		while (!bvhStack.empty()) {

			uint32_t currentIndex = bvhStack.top();
			bvhStack.pop();

			uint32_t parentIndex = octree[currentIndex].parentIndex;
			if (parentIndex != UINT32_MAX) {           // check were not at the root
				octree[parentIndex].nodeExtents.extendBounds(octree[currentIndex].nodeExtents);
			}
		}
	}

	void Octree::insertObjectIntoTreeIterative(const Extents& extents)
	{
		std::stack<IterativeTreeInfo> treeStack;

		uint32_t currentIndex = 0;
		uint8_t currentDepth = 0;

		// start at the root
		treeStack.push({ currentIndex, currentDepth, bbox, extents });                          // push root data onto stack

		while (!treeStack.empty()) {
			IterativeTreeInfo stackInfo = treeStack.top();

			currentIndex = stackInfo.index;
			currentDepth = stackInfo.depth;
			BoundingBox currentBBox = stackInfo.bbox;
			Extents currentExtent = stackInfo.extent;
			treeStack.pop();

			if (octree[currentIndex].isLeaf) {
				if (octree[currentIndex].nodeExtentsList.empty() || currentDepth == maxTreeDepth) {
					octree[currentIndex].nodeExtentsList.push_back(currentExtent);
				}
				else {
					octree[currentIndex].isLeaf = false;            // set as internal branch node

					 // split into 8 new child cells and transfer the current objects - this will only be one object!
					while (octree[currentIndex].nodeExtentsList.size()) {
						treeStack.push({ currentIndex, currentDepth, currentBBox, octree[currentIndex].nodeExtentsList.back() });
						octree[currentIndex].nodeExtentsList.pop_back();
					}
					// and also insert the new object into the tree
					treeStack.push({ currentIndex, currentDepth, currentBBox, currentExtent });
				}
			}
			else {
				// if it's not a leaf, then treat as an internal branch node
				// interal node - calculate the center of the object and node to determine which child to use -
				// child nodes are represented spatially in xyz dimensions
				// One possible oprimisation is to use Morton codes as illustarted here: https://devblogs.nvidia.com/thinking-parallel-part-iii-tree-construction-gpu/
				OEMaths::vec3f objCentroid{ currentExtent.calculateCentroid() };
				OEMaths::vec3f nodeCentroid = { currentBBox.calculateCentroid() };
				BoundingBox childBBox;
				uint8_t childIndex = 0;

				// check the x-axis
				if (objCentroid.getX() > nodeCentroid.getX()) {
					childBBox.bounds.min.setX(nodeCentroid.getX());
					childBBox.bounds.max.setX(currentBBox.bounds.max.getX());
					childIndex = 4;
				}
				else {
					childBBox.bounds.min.setX(currentBBox.bounds.min.getX());
					childBBox.bounds.max.setX(nodeCentroid.getX());
				}
				// check the y-axis
				if (objCentroid.getY() > nodeCentroid.getY()) {
					childBBox.bounds.min.setY(nodeCentroid.getY());
					childBBox.bounds.max.setY(currentBBox.bounds.max.getY());
					childIndex += 2;
				}
				else {
					childBBox.bounds.min.setY(currentBBox.bounds.min.getY());
					childBBox.bounds.max.setY(nodeCentroid.getY());
				}
				// check the z-axis
				if (objCentroid.getZ() > nodeCentroid.getZ()) {
					childBBox.bounds.min.setZ(nodeCentroid.getZ());
					childBBox.bounds.max.setZ(currentBBox.bounds.max.getZ());
					childIndex += 1;
				}
				else {
					childBBox.bounds.min.setZ(currentBBox.bounds.min.getZ());
					childBBox.bounds.max.setZ(nodeCentroid.getZ());
				}

				// if child doesn't exsist, create a new one (no child is signifyed by a zero value). Otherwise, keep traversing the tree
				if (octree[currentIndex].childNode[childIndex] == 0) {
					octree[currentIndex].childNode[childIndex] = nextFreeIndex++;

					// push new child node into tree
					octree.push_back({ currentIndex });
				}

				// add to stack for processing - move down (up?) a branch
				uint8_t nextDepth = currentDepth + 1;
				treeStack.push({ octree[currentIndex].childNode[childIndex], nextDepth, childBBox, currentExtent });
			}
		}
	}


	// BVH functions ============================================================================================================================

	void BVH::setupOctree()
	{

		Extents sceneExtents;
		primitiveExtents.resize(primitives.size());

		for (uint32_t primIndex = 0; primIndex < primitives.size(); ++primIndex) {
			
			assert(primIndex < primitiveExtents.size());

			for (uint8_t planeIndex = 0; planeIndex < Extents::boundingPlaneCount; ++planeIndex) {
				
				// calculate the angle between the plane normal and the primitive point point for min values
				float distance = planeNormals[planeIndex].dot(primitives[primIndex].min);

				// set dnear
				if (distance < primitiveExtents[primIndex].distance[planeIndex].near) {
					primitiveExtents[primIndex].distance[planeIndex].near = distance;
				}
				// set dfar
				if (distance > primitiveExtents[primIndex].distance[planeIndex].far) {
					primitiveExtents[primIndex].distance[planeIndex].far = distance;
				}

				// calculate the angle between the plane normal and the primitive point point for max values
				distance = planeNormals[planeIndex].dot(primitives[primIndex].max);

				// set dnear
				if (distance < primitiveExtents[primIndex].distance[planeIndex].near) {
					primitiveExtents[primIndex].distance[planeIndex].near = distance;
				}
				// set dfar
				if (distance > primitiveExtents[primIndex].distance[planeIndex].far) {
					primitiveExtents[primIndex].distance[planeIndex].far = distance;
				}
			}

			// update total scene extents
			sceneExtents.extendBounds(primitiveExtents[primIndex]);
			primitiveExtents[primIndex].primitiveIndex = primIndex;

		}

		// finally, create root node width root node extents set as the total scene extents
		octree = std::make_unique<Octree>(sceneExtents);
	}


	void BVH::buildTree()
	{
		if (primitives.empty()) {
			return;
		}

		// create new BVH octree - adjust total scene extents based on each primitive type added.
		setupOctree();

		// bvh tree is built in two steps - first step is to generate a spatial representation of the world in an octree for all primitive types.
		// The second step is to work form the leaves in a bottom up fashion and adjust the bounding boxes in relation to the
		// objects within the leaves.
		for (auto& primitive : primitiveExtents) {
			octree->insertObjectIntoTreeIterative(primitive);
		}

		// build the tree - adjust bounding boxes according to the primitves contained within the cell
		octree->buildBvhIterative();
	}

	void BVH::addPrimitive(OEMaths::vec3f min, OEMaths::vec3f max, uint32_t mesh_index, uint32_t primitive_index)
	{
		primitives.push_back({ mesh_index, primitive_index, min, max });
	}

	void BVH::clear()
	{
		octree = nullptr;
		primitives.clear();
		primitiveExtents.clear();
	}

	bool BVH::isEmpty() const
	{
		return octree == nullptr;
	}

	uint32_t BVH::octreeSize() const
	{
		return static_cast<uint32_t>(octree->octree.size());
	}


	uint32_t BVH::primitiveExtentsSize() const
	{
		return static_cast<uint32_t>(primitiveExtents.size());
	}

}