#pragma once


#include <unordered_map>
#include <vector>

// forward decleration
namespace VulkanAPI
{
class CmdBuffer;
}

namespace OmegaEngine
{

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

	float depth; // for transparency;
};

/**
* @brief All the information required to render the item to a cmd buffer. This is set by the renderer update function.
*/
struct RenderableQueueInfo
{
	// render callback function
	void (*renderFunction)(void *, VulaknAPI::CmdBuffer&, void *renderableData);
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
    enum class Type
    {
        Colour,
        EarlyDepth,
        Lighting
    };
    
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
    
	void push(RenderableQueueInfo &renderInfo)
	{
		renderables.emplace_back(renderInfo);
	}

	static SortKey createSortKey(Layer layer, size_t materialId, uint64_t variantId);
	
	void sortAll();

	/**
	* @brief Returns all renderables present in the queue
	* @param sort Specifies whether the queue should be sorted before fetching
	* @return A vector containing all renderables present in the queue
	*/
	std::vector<RenderableQueueInfo> getAll(bool sort);

	/**
	* @brief Returns all renderables between the specified range
	* @param start The starting index in the range
	* @param end The end index in the range
	* @param sort Specifies whether the queue should be sorted before fetching
	* @return A vector containing the renderables in the specified range
	*/
	std::vector<RenderableQueueInfo> getRange(const size_t start, const size_t end, Type type);

	friend class Renderer;

private:
    
	// ordered by queue type
	std::vector<RenderableQueueInfo> renderables;

};
} // namespace OmegaEngine
