#include "vkFFT.h"
#include "VulkanCore/VulkanEngine.h"

vkFFT::vkFFT(VulkanEngine *engine) :
	p_vkEngine(engine)
{
}


vkFFT::~vkFFT()
{
}

void vkFFT::CreateBuffers()
{
	// ubo buffer
	m_uboBuffer.CreateBuffer(sizeof(UboLayout), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_vkEngine);

	// generate 6 buffers for each phase / stride
	int i_stride = 512 * 512 / 8;
	int o_stride = i_stride;
	int p_stride = 512;
	float phaseBase = -PI_2 / (512.0f * 512.0f);
	UboLayout ubo;

	for (uint32_t c = 0; c < 6; ++c) {

		UboData data;
		data.i_stride = i_stride;
		data.o_stride = o_stride;
		data.p_stride = p_stride;
		data.phaseBase = phaseBase;
		ubo.data[c] = data;

		i_stride /= 8;
		phaseBase *= 8.0f;

		if (c == 3) {
			o_stride /= 512.0;
			p_stride = 1;
		}
	}

	// transfer to local device
	m_uboBuffer.MapBuffer<UboLayout>(ubo, p_vkEngine->GetDevice());

	// create ssbo buffers - for destination (d(z))
	m_ssboDstBuffer.CreateBuffer(sizeof(glm::vec2) * (512 * 512), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, p_vkEngine);

}

void vkFFT::GenerateFFTCmdBuffer(VkBuffer &srcBuffer)
{
	
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

	m_cmdBuffer = vkUtility->CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->GetComputeCmdPool());

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	VK_CHECK_RESULT(vkCreateFence(p_vkEngine->GetDevice(), &fenceInfo, nullptr, &m_fence));

	// Ensure that the compute shader has finished computing the h(t) values before continuing with FFT calculations 
	VkBufferMemoryBarrier computeBarrier = {};
	computeBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	computeBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	computeBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	computeBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	computeBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	computeBarrier.size = VK_WHOLE_SIZE;

	std::array<VkBufferMemoryBarrier, 2> bufferBarriers = {};
	computeBarrier.buffer = srcBuffer;
	bufferBarriers[0] = computeBarrier;
	computeBarrier.buffer = m_ssboDstBuffer.buffer;
	bufferBarriers[1] = computeBarrier;

	vkCmdPipelineBarrier(m_cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(), 0, nullptr);

	vkCmdBindPipeline(m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineInfo.pipeline);
	vkCmdBindDescriptorSets(m_cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineInfo.layout, 0, 1, &descriptors.set, 0, nullptr);

	for (uint32_t c = 0; c < 6; ++c) {
		
		uint32_t push = c;
		vkCmdPushConstants(m_cmdBuffer, pipelineInfo.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &push);
		vkCmdDispatch(m_cmdBuffer, ((512 * 512)) / 128, 1, 1);
	}

	// Ensure compute shader has finished before rendering
	for (uint32_t c = 0; c < bufferBarriers.size(); ++c) {
		bufferBarriers[c].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		bufferBarriers[c].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		bufferBarriers[c].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		bufferBarriers[c].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	}

	vkCmdPipelineBarrier(m_cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(), 0, nullptr);
	vkEndCommandBuffer(m_cmdBuffer);
}

void vkFFT::PrepareFFTDescriptorSets(VulkanBuffer &srcBuffer)
{
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);
	
	// =============================================== descriptor pool =======================================================================
	std::array<VkDescriptorPoolSize, 2> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 1;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descrPoolSize[1].descriptorCount = 2;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->GetDevice(), &createInfo, nullptr, &descriptors.pool));

	// =============================================== descriptor layout ====================================================================
	std::array<VkDescriptorSetLayoutBinding, 3> layoutBinding;
	layoutBinding[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);					// bindings for the dst SSBO - d(z)
	layoutBinding[1] = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);					// bindings for the src SSBO - h(t)	
	layoutBinding[2] = vkUtility->InitLayoutBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);					// 	bindings for the UBO	
																						
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBinding.size());
	layoutInfo.pBindings = layoutBinding.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &descriptors.layout));

	// ================================================ descriptor sets =====================================================================
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &descriptors.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->GetDevice(), &allocInfo, &descriptors.set));

	std::array<VkDescriptorBufferInfo, 3> bufferInfo;
	bufferInfo[0] = vkUtility->InitBufferInfoDescriptor(srcBuffer.buffer, 0, srcBuffer.size);				// h(t) - computed from compute shader
	bufferInfo[1] = vkUtility->InitBufferInfoDescriptor(m_ssboDstBuffer.buffer, 0, m_ssboDstBuffer.size);	// d(z)
	bufferInfo[2] = vkUtility->InitBufferInfoDescriptor(m_uboBuffer.buffer, 0, m_uboBuffer.size);			// ubo

	std::array<VkWriteDescriptorSet, 3> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(descriptors.set, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo[0]);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(descriptors.set, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo[1]);
	writeDescrSet[2] = vkUtility->InitDescriptorSet(descriptors.set, 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo[2]);

	vkUpdateDescriptorSets(p_vkEngine->GetDevice(), static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);

	delete vkUtility;
}

void vkFFT::PrepareFFTPipeline()
{
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(uint32_t);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	VkPushConstantRange pushConstantArray[] = { pushConstant };

	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &descriptors.layout;
	layoutInfo.pPushConstantRanges = &pushConstant;
	layoutInfo.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &pipelineInfo.layout));

	shader = vkUtility->InitShaders("terrain/water/fft-comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

	VkComputePipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.layout = pipelineInfo.layout;
	createInfo.stage = shader;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	VK_CHECK_RESULT(vkCreateComputePipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipelineInfo.pipeline));

	delete vkUtility;
}

void vkFFT::SubmitFFTCompute()
{
	// submit FFT calculations
	vkResetFences(p_vkEngine->GetDevice(), 1, &m_fence);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_cmdBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(p_vkEngine->GetComputeQueue(), 1, &submitInfo, m_fence));

	vkWaitForFences(p_vkEngine->GetDevice(), 1, &m_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(p_vkEngine->GetDevice(), 1, &m_fence);
}
