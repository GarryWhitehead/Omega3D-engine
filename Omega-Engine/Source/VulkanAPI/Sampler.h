#pragma once

#include "VulkanAPI/Common.h"

namespace VulkanAPI
{
// forward declerations
struct VkContext;

enum class SamplerType
{
	Clamp,
	Wrap,
	TrilinearWrap,
	LinearWrap,
	TriLinearClamp,
	LinearClamp,
	NotDefined
};

class Sampler
{

public:
	
	Sampler(SamplerType type);
	~Sampler();

	static SamplerType getSamplerType(const vk::SamplerAddressMode mode, const vk::Filter filter);
	static SamplerType getDefaultSampler();

	void build(VkContext& context);

	vk::Sampler& get()
	{
		return sampler;
	}

private:
    
	void createSampler(VkContext& context, vk::SamplerAddressMode addressMode, vk::Filter filter,
	                   vk::SamplerMipmapMode mipMapMode, bool compare_op);

private:
    
	SamplerType type;
	vk::Sampler sampler;
};

} // namespace VulkanAPI
