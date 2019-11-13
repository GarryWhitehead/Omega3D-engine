#pragma once

#include "VulkanAPI/CommandBuffer.h"
#include "VulkanAPI/Common.h"

#include <unordered_map>
#include <vector>

namespace OmegaEngine
{
// forward declerations
class ThreadPool;
struct ProgramState;
class ComponentInterface;
enum class RenderTypes;

enum class RenderQueueType
{
	Graphics,
	Present
};

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

// all the information required to render
struct RenderableQueueInfo
{
	// render callback function
	void (*renderFunction)(void *, void *renderableData);
	void *renderableHandle;

	// data specific to the renderable - mainly drawing information
	void *renderableData;

	// the point in the render this will be drawn
	SortKey sortingKey;
};

class RenderQueue
{
public:
	RenderQueue();
	~RenderQueue();

	void addRenderableToQueue(RenderableQueueInfo &renderInfo)
	{
		renderables.emplace_back(renderInfo);
	}

	static SortKey createSortKey(RenderStage layer, uint32_t materialId, RenderTypes shaderId);
	
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
	std::vector<RenderableQueueInfo> getRange(const size_t start, const size_t end);

private:
	// ordered by queue type
	std::vector<RenderableQueueInfo> renderables;

};
} // namespace OmegaEngine