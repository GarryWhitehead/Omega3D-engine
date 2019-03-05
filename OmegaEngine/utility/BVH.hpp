
#pragma once

#include "OEMaths/OEMaths.h"

#include <array>
#include <limits>
#include <stack>
#include <vector>

namespace OmegaEngine
{

	struct ExtentDistance3D
	{
		OEMaths::vec3f min;
		OEMaths::vec3f max;
	};

	struct BoundingBox
	{
		BoundingBox();

		/// @brief calculates the middle co-ords of the current bounding box in R3 space
		OEMaths::vec3f calculateCentroid() const;

		/// min and max extents of bounding box in R3 space
		struct BoxDimensions
		{
			OEMaths::vec3f min;
			OEMaths::vec3f max;
		} bounds;
	};

	struct Extents
	{
		static const uint8_t boundingPlaneCount = 3;

		Extents();

		/// \brief tests for intersection between line (ray) and plane based on distance of near and far extents and ray origin/direction
		/// \param tNear the current near distance
		/// \param tFar the current far distance
		/// \param normDotOrig a pointer to an array containing the pre-computed ray origin . normal vector for each plane (xyz)
		/// \param normDotOrig a pointer to an array containing the pre-computed ray direction . normal vector for each plane (xyz)
		/// \return if the ray intersects with the plane, return true, otherwise false
		bool testForPlaneIntersect(float& tNear, float& tFar, const float* normDotOrig, const float* normDotDir);

		/// \brief checks the current extents against the extents of the primitive extents passed, and adjusts if the primitive extents are greater
		/// \param extents The extents of the primitive
		void extendBounds(const Extents& extents);

		/// \brief calculates the center position of the current extents
		OEMaths::vec3f calculateCentroid();

		/// \brief resets all extents to infinity.
		void reset();

		struct Distance
		{
			float near;
			float far;
		} distance[boundingPlaneCount];                   ///< tNear and tFar values for two plane "slabs"


		/// \brief This is the index into the primitive extents data container for this particular primitive
		uint32_t primitiveIndex;
	};

	class Octree
	{
	public:

		static const uint8_t maxTreeDepth = 16;
		static const uint8_t childNodeCount = 8;

		struct Node
		{
			Node() :
				parentIndex(UINT32_MAX),
				isLeaf(true)
			{}

			Node(uint32_t parent) :
				parentIndex(parent),
				isLeaf(true)
			{}

			Node(Extents e) :
				parentIndex(UINT32_MAX),
				nodeExtents(e),
				isLeaf(true)
			{}

			uint32_t parentIndex;
			std::array<uint32_t, childNodeCount> childNode{ { 0 } };
			std::vector<Extents> nodeExtentsList;
			Extents nodeExtents;
			bool isLeaf;
		};

		/// \brief this is used for creating a min heep using priority_queue by using the ray-extents distance as a parameter.
		struct NodeElement
		{
			NodeElement(uint32_t index, float dist) :
				nodeIndex(index),
				distance(dist)
			{}

			uint32_t nodeIndex;
			float distance;         ///< distance(t) from the ray to the extents of the particular node in this element

			/// \brief the priority_queue will be used as a min heap - smaller distances will be pulled first
			friend bool operator<(const NodeElement& a, const NodeElement& b) { return a.distance > b.distance; }
		};

		/// iterative stack data
		struct IterativeTreeInfo
		{
			uint32_t index;
			uint8_t depth;
			BoundingBox bbox;
			Extents extent;
		};

		Octree(const Extents& sceneExtents);

		/// \brief Builds an octree, inserting the positional data for a primitive into the octree. The leaf in the octree in which the primitive is inserted depends on its locarion in R3 space. This function builds a spatial representation of the 3D world. The data is store as a linear array which should hopefully improve cache performance!
		/// @param extents The extents of the primitive to insert into the tree
		void insertObjectIntoTreeIterative(const Extents& extents);

		/// \brief Uses the current octree, and build a bounding volume heirachy based on this. This is basically a axis-aligned box with extents which encompasses the total volume at the particular leaf or internal node at each depth of the tree. Obviously, the tree must be built first!
		void buildBvhIterative();

		uint32_t nextFreeIndex;
		std::vector<Node> octree;           ///< octree nodes container

	private:

		BoundingBox bbox;                   ///< bounding box which is the total volume of the entire scene
	};

	class BVH
	{
	public:

		/// @brief The normals for each plane - using 3 planes - for complex shapes, 7 planes could be used...... possible easy optimistaion if using complex shapes !!
		const OEMaths::vec3f planeNormals[Extents::boundingPlaneCount] =
		{
			{ 1.0f, 0.0f, 0.0f },         // x-plane normal
			{ 0.0f, 1.0f, 0.0f },         // y-plane normal
			{ 0.0f, 0.0f, 1.0f }          // z-plane normal
		};



		/// \brief calculate the min and max distances for a primtiive for each plane normal. The primitive extents MUST be pushed into the
		/// primitiveExtents container before calling this function
		/// \param primitiveIndex The index of the primitive for which the extents are to be calculated
		void generateExtentsForPrimitive(const uint32_t primitiveIndex);

		/// \brief calculate the extents of all 3D primitives and create a new octree based on the entrire scene extents
		void setupOctree();

		/// \brief the main meat of the bvh. builds an octree from the primitives that have been pushed. The bvh is then generated from the octree
		void buildTree();

		/// \brief clears all the 2D and 3D primitive data and destroys the octree array
		void clear();

		/// \brief simple helper which tells the user if the tree is empty
		bool isEmpty() const;

		/// \brief returns the size of the octree
		uint32_t octreeSize() const;

		/// \brief returns the number of 3D primitives stored
		uint32_t primitive3DSize() const;

		/// \brief returns the number of extents stored. This should be the same as *primitive3DSize*. If it isn't, something has gone wrong!
		uint32_t primitiveExtentsSize() const;

	private:

		std::unique_ptr<Octree> octree;

		// 3D data
		std::vector<Extents> primitiveExtents;
		std::vector<Primitive3D> primitiveData3D;

	};

}
