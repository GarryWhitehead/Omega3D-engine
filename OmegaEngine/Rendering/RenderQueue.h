#pragma once


#include <array>
#include <unordered_map>
#include <vector>

// forward decleration
namespace VulkanAPI
{
class CmdBuffer;
}

namespace OmegaEngine
{

// forward declerations
class RGraphContext;

struct SortKey
{
	union {
		struct
		{
			// in order of sorting importance
			uint64_t layerId : 4;
			uint64_t shaderId : 12;
			uint64_t textureId : 12;
			uint64_t depthId : 12;
		} s;

		uint64_t flags;

	} u;

	float depth;    // for transparency;
};

/**
* @brief All the information required to render the item to a cmd buffer. This is set by the renderer update function.
*/
struct RenderableQueueInfo
{
	// render callback function
	void (*renderFunction)(VulkanAPI::CmdBuffer&, void*, RGraphContext&);
	void* renderableHandle;

	// data specific to the renderable - mainly drawing information
	void* renderableData;

	// the point in the render this will be drawn
	SortKey sortingKey;
};

class RenderQueue
{
public:
	/**
     * @brief The type of queue to use when drawing. Only one at the moment!!
     */
	enum Partition
	{
		Colour,
		EarlyDepth,
		Lighting,
		Count
	};

	/**
	* The render queue is partitioned into different renderable types
	*/
	enum class Layer
	{
		Default,
		Front,
		Back
	};

	RenderQueue();
	~RenderQueue();

	// not copyable
	RenderQueue(const RenderQueue&) = delete;
	RenderQueue& operator=(const RenderQueue&) = delete;

	void reset();

	void pushRenderables(std::vector<RenderableQueueInfo>& newRenderables, const Partition part);

	static SortKey createSortKey(Layer layer, size_t materialId, uint64_t variantId);

	void sortPartition(const Partition part);

	void sortAll();

	/**
	* @brief Returns all renderables in the specified partition
	* @param part Specifies whether the queue should be sorted before fetching
	* @return A vector containing the renderables in the specified range
	*/
	std::vector<RenderableQueueInfo> getPartition(const RenderQueue::Partition part);

	// ================== helpers ======================
	size_t size(const Partition part)
	{
		return partSizes[part];
	}

	friend class Renderer;

private:
	// the offset/size for each partition
	std::array<size_t, Partition::Count> partSizes;
	std::array<size_t, Partition::Count> partOffsets;

	// ordered by queue type
	std::vector<RenderableQueueInfo> renderables;
};
}    // namespace OmegaEngine
