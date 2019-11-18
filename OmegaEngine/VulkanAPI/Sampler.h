#pragma once
#include "VulkanAPI/Common.h"

namespace VulkanAPI
{
// forward declerations
class VkContext;

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
	Sampler();
	Sampler(vk::Device device, SamplerType type);
	~Sampler();

	static SamplerType getSamplerType(const vk::SamplerAddressMode mode, const vk::Filter filter);
	static SamplerType getDefaultSampler();

	void create(VkContext& context, SamplerType type);

	vk::Sampler &getSampler()
	{
		return sampler;
	}

private:
    
	void createSampler(VkContext& context, vk::SamplerAddressMode addressMode, vk::Filter filter,
	                   vk::SamplerMipmapMode mipMapMode, bool compare_op);

private:
    
	vk::Sampler sampler;
};

} // namespace VulkanAPI
