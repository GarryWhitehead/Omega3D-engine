#include "vkFFT.h"
#include "VulkanCore/VulkanEngine.h"
#include "utility/RandomNumber.h"

vkFFT::vkFFT(VulkanEngine *engine, VkMemoryManager *memory) :
	p_vkEngine(engine),
	p_vkMemory(memory)
{
}


vkFFT::~vkFFT()
{
}

void vkFFT::CreateBuffers()
{
	uint32_t log2N = log(N) / log(2);

	// spectrum compute buffers
	ubo_spectrum = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(SpecUboBuffer));
	ssbo_dxyz = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(glm::vec2) * (N * N * 3));		// using one buffer and offsets to access each dt buffer

	// twiidle compute shader
	// twiddle texture
	m_images.butterfly.PrepareImage(VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, log2N, N, p_vkEngine);

	// FFT compute
	// pingpong ssbo
	ssbo_pingpong = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, sizeof(glm::vec2) * (N * N * 3));

	// displacement shader - texture
	m_images.displacement.PrepareImage(VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, N, N, p_vkEngine);
	ubo_displacement = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(DisplacementUbo));

	// setup displacement ubo now as the data will be static
	std::vector<DisplacementUbo> ubo(1);
	ubo[0].N = N;
	ubo[0].offset_dx = OFFSET_DX;
	ubo[0].offset_dy = OFFSET_DY;
	ubo[0].offset_dz = OFFSET_DZ;
	ubo[0].choppyFactor = 1.5f;
	p_vkMemory->MapDataToSegment<DisplacementUbo>(ubo_displacement, ubo);

	// normal compute - texture storage
	m_images.normal.PrepareImage(VK_FORMAT_R16G16B16A16_SFLOAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, N, N, p_vkEngine);

	// normal compute ubo
	ubo_normal = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, sizeof(NormalUbo));

	std::vector<NormalUbo> normUbo(1);
	normUbo[0].choppiness = 1.0f;
	normUbo[0].gridLength = vkFFT::N / 1000;
	p_vkMemory->MapDataToSegment<NormalUbo>(ubo_normal, normUbo);
}

void vkFFT::GenerateButterflyCmdBuffer()
{

	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

	VkCommandBuffer cmdBuffer = vkUtility->CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->GetComputeCmdPool());

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fftButterfly.pipelineInfo.pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fftButterfly.pipelineInfo.layout, 0, 1, &m_fftButterfly.descriptors.set, 0, nullptr);

	uint32_t log2N = log(N) / log(2);
	uint32_t groupSize = N / 16;
	uint32_t push = N;

	vkCmdPushConstants(cmdBuffer, m_fftButterfly.pipelineInfo.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &push);
	vkCmdDispatch(cmdBuffer, log2N, groupSize, 1);

	vkUtility->SubmitCmdBufferToQueue(cmdBuffer, p_vkEngine->GetComputeQueue(), p_vkEngine->GetComputeCmdPool());

	delete vkUtility;
}

void vkFFT::GenerateSpectrumCmdBuffer(VkCommandBuffer cmdBuffer)
{
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fftSpectrum.pipelineInfo.pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fftSpectrum.pipelineInfo.layout, 0, 1, &m_fftSpectrum.descriptors.set, 0, nullptr);

	uint32_t groupSize = N * N / 16;
	vkCmdDispatch(cmdBuffer, groupSize, groupSize, 1);
}

void vkFFT::GenerateFFTCmdBuffer(VkCommandBuffer cmdBuffer)
{
	// ensure that the spectrum compute has finished before reading from the buffers
	VkBufferMemoryBarrier memBarrier = {};
	memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;								// Compute shader has finished writes to the buffer
	memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarrier.buffer = p_vkMemory->blockBuffer(ssbo_dxyz.block_id);
	memBarrier.size = ssbo_dxyz.size;

	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &memBarrier, 0, nullptr);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fft.pipelineInfo.pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fft.pipelineInfo.layout, 0, 1, &m_fft.descriptors.set, 0, nullptr);

	uint32_t groupSize = N / 16;
	uint32_t log2N = log(N) / log(2);

	// horizontal DFT
	FFTPushData pushData;
	m_pingpongID = 0;
	pushData.N = N;	
	for (uint32_t c = 0; c < log2N; ++c) {

		pushData.direction = 0.0f;	// 0 = horizontal; 1 = vertical
		pushData.pingpong = m_pingpongID;
		pushData.stage = c;

		pushData.offset = OFFSET_DY;
		vkCmdPushConstants(cmdBuffer, m_fft.pipelineInfo.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FFTPushData), &pushData);
		vkCmdDispatch(cmdBuffer, groupSize, groupSize, 1);

		pushData.offset = OFFSET_DX;
		vkCmdPushConstants(cmdBuffer, m_fft.pipelineInfo.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FFTPushData), &pushData);
		vkCmdDispatch(cmdBuffer, groupSize, groupSize, 1);

		pushData.offset = OFFSET_DZ;
		vkCmdPushConstants(cmdBuffer, m_fft.pipelineInfo.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FFTPushData), &pushData);
		vkCmdDispatch(cmdBuffer, groupSize, groupSize, 1);

		m_pingpongID ^= 1;
	}

	// vertical DFT
	m_pingpongID = 0;
	pushData.direction = 1.0f;

	for (uint32_t c = 0; c < log2N; ++c) {
		
		pushData.pingpong = m_pingpongID;
		pushData.stage = c;

		pushData.offset = OFFSET_DY;
		vkCmdPushConstants(cmdBuffer, m_fft.pipelineInfo.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FFTPushData), &pushData);
		vkCmdDispatch(cmdBuffer, groupSize, groupSize, 1);

		pushData.offset = OFFSET_DX;
		vkCmdPushConstants(cmdBuffer, m_fft.pipelineInfo.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FFTPushData), &pushData);
		vkCmdDispatch(cmdBuffer, groupSize, groupSize, 1);

		pushData.offset = OFFSET_DZ;
		vkCmdPushConstants(cmdBuffer, m_fft.pipelineInfo.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FFTPushData), &pushData);
		vkCmdDispatch(cmdBuffer, groupSize, groupSize, 1);

		m_pingpongID ^= 1;
	}

	memBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;								
	memBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT; 
	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &memBarrier, 0, nullptr);
}

void vkFFT::GeneratDisplacementCmdBuffer(VkCommandBuffer cmdBuffer)
{
	// ensure that the spectrum compute has finished before reading from the buffers
	VkBufferMemoryBarrier memBarrier = {};
	memBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;								// Compute shader has finished writes to the buffer
	memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	memBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	memBarrier.buffer = p_vkMemory->blockBuffer(ssbo_dxyz.block_id);
	memBarrier.size = ssbo_dxyz.size;

	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &memBarrier, 0, nullptr);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fftDisplacement.pipelineInfo.pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fftDisplacement.pipelineInfo.layout, 0, 1, &m_fftDisplacement.descriptors.set, 0, nullptr);

	uint32_t groupSize = N * N / 16;

	uint32_t push = 0;		// use the last buffer called - this will depend on the FFT resolution (N)
	vkCmdPushConstants(cmdBuffer, m_fft.pipelineInfo.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &push);
	vkCmdDispatch(cmdBuffer, groupSize, groupSize, 1);

	memBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	memBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	vkCmdPipelineBarrier(cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1, &memBarrier, 0, nullptr);
}

void vkFFT::GenerateNormalCmdBuffer(VkCommandBuffer cmdBuffer)
{	
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fftNormal.pipelineInfo.pipeline);
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fftNormal.pipelineInfo.layout, 0, 1, &m_fftNormal.descriptors.set, 0, nullptr);

	uint32_t groupSize = N * N / 16;
	vkCmdDispatch(cmdBuffer, groupSize, groupSize, 1);
}

void vkFFT::SubmitFFTCompute()
{
	
	vkResetFences(p_vkEngine->GetDevice(), 1, &m_computeFence);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_computeCmdBuffer;

	VK_CHECK_RESULT(vkQueueSubmit(p_vkEngine->GetComputeQueue(), 1, &submitInfo, m_computeFence));

	vkWaitForFences(p_vkEngine->GetDevice(), 1, &m_computeFence, VK_TRUE, UINT64_MAX);
	vkResetFences(p_vkEngine->GetDevice(), 1, &m_computeFence);
}

void vkFFT::PrepareSpecDescriptorSets()
{
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);
	
	// =============================================== descriptor pool =======================================================================
	std::array<VkDescriptorPoolSize, 2> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 1;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;		// h0(k) and h0(-k) inputs and d(x,y,z) output
	descrPoolSize[1].descriptorCount = 3;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->GetDevice(), &createInfo, nullptr, &m_fftSpectrum.descriptors.pool));

	// =============================================== descriptor layout ====================================================================
	std::array<VkDescriptorSetLayoutBinding, 4> layoutBinding;
	layoutBinding[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);			// h0(k)			
	layoutBinding[1] = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);			// h0(-k)
	layoutBinding[2] = vkUtility->InitLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);			// dt(x,y,z)
	layoutBinding[3] = vkUtility->InitLayoutBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);					
																						
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBinding.size());
	layoutInfo.pBindings = layoutBinding.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_fftSpectrum.descriptors.layout));

	// ================================================ descriptor sets =====================================================================
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_fftSpectrum.descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_fftSpectrum.descriptors.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->GetDevice(), &allocInfo, &m_fftSpectrum.descriptors.set));

	std::array<VkDescriptorBufferInfo, 4> bufferInfo;
	bufferInfo[0] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(h0k_map.block_id), h0k_map.offset, h0k_map.size);									// h0(k) - 
	bufferInfo[1] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(h0minusk_map.block_id), h0minusk_map.offset, h0minusk_map.size);					// h0(-k)
	bufferInfo[2] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(ssbo_dxyz.block_id), ssbo_dxyz.offset, ssbo_dxyz.size);								// h0(-k)
	bufferInfo[3] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(ubo_spectrum.block_id), ubo_spectrum.offset, ubo_spectrum.size);					// ubo

	std::array<VkWriteDescriptorSet, 4> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_fftSpectrum.descriptors.set, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo[0]);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(m_fftSpectrum.descriptors.set, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo[1]);
	writeDescrSet[2] = vkUtility->InitDescriptorSet(m_fftSpectrum.descriptors.set, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo[2]);
	writeDescrSet[3] = vkUtility->InitDescriptorSet(m_fftSpectrum.descriptors.set, 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo[3]);

	vkUpdateDescriptorSets(p_vkEngine->GetDevice(), static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);

	delete vkUtility;
}

void vkFFT::PrepareButterflyDescriptorSets()
{
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

	// =============================================== descriptor pool =======================================================================
	std::array<VkDescriptorPoolSize, 2> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;		// bit reversed
	descrPoolSize[0].descriptorCount = 1;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;		// butterfly output
	descrPoolSize[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->GetDevice(), &createInfo, nullptr, &m_fftButterfly.descriptors.pool));

	// =============================================== descriptor layout ====================================================================
	std::array<VkDescriptorSetLayoutBinding, 2> layoutBinding;
	layoutBinding[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);			// h0(k)			
	layoutBinding[1] = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);			// h0(-k)

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBinding.size());
	layoutInfo.pBindings = layoutBinding.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_fftButterfly.descriptors.layout));

	// ================================================ descriptor sets =====================================================================
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_fftButterfly.descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_fftButterfly.descriptors.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->GetDevice(), &allocInfo, &m_fftButterfly.descriptors.set));

	std::array<VkDescriptorBufferInfo, 1> bufferInfo;
	bufferInfo[0] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(bitrev_map.block_id), bitrev_map.offset, bitrev_map.size);														// h0(k) - 
	std::array<VkDescriptorImageInfo, 1> imageInfo;
	imageInfo[0] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_GENERAL, m_images.butterfly.imageView, m_images.butterfly.texSampler);

	std::array<VkWriteDescriptorSet, 2> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_fftButterfly.descriptors.set, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo[0]);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(m_fftButterfly.descriptors.set, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageInfo[0]);

	vkUpdateDescriptorSets(p_vkEngine->GetDevice(), static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);

	delete vkUtility;
}

void vkFFT::PrepareFFTDescriptorSets()
{
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

	// =============================================== descriptor pool =======================================================================
	std::array<VkDescriptorPoolSize, 2> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;		
	descrPoolSize[0].descriptorCount = 1;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descrPoolSize[1].descriptorCount = 2;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->GetDevice(), &createInfo, nullptr, &m_fft.descriptors.pool));

	// =============================================== descriptor layout ====================================================================
	std::array<VkDescriptorSetLayoutBinding, 3> layoutBinding;
	layoutBinding[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);			// butterfly image - input		
	layoutBinding[1] = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);			// d(x,y,z)
	layoutBinding[2] = vkUtility->InitLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);			// pingpong

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBinding.size());
	layoutInfo.pBindings = layoutBinding.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_fft.descriptors.layout));

	// ================================================ descriptor sets =====================================================================
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_fft.descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_fft.descriptors.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->GetDevice(), &allocInfo, &m_fft.descriptors.set));
						
	std::array<VkDescriptorBufferInfo, 2> bufferInfo;
	bufferInfo[0] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(ssbo_dxyz.block_id), ssbo_dxyz.offset, ssbo_dxyz.size);
	bufferInfo[1] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(ssbo_pingpong.block_id), ssbo_pingpong.offset, ssbo_pingpong.size);
	std::array<VkDescriptorImageInfo, 1> imageInfo;
	imageInfo[0] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_GENERAL, m_images.butterfly.imageView, m_images.butterfly.texSampler);		// butterfly image

	std::array<VkWriteDescriptorSet, 3> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_fft.descriptors.set, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageInfo[0]);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(m_fft.descriptors.set, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo[0]);
	writeDescrSet[2] = vkUtility->InitDescriptorSet(m_fft.descriptors.set, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo[1]);
	
	vkUpdateDescriptorSets(p_vkEngine->GetDevice(), static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);

	delete vkUtility;
}

void vkFFT::PrepareDisplacementDescriptorSets()
{
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

	// =============================================== descriptor pool =======================================================================
	std::array<VkDescriptorPoolSize, 3> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descrPoolSize[0].descriptorCount = 1;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descrPoolSize[1].descriptorCount = 2;
	descrPoolSize[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->GetDevice(), &createInfo, nullptr, &m_fftDisplacement.descriptors.pool));

	// =============================================== descriptor layout ====================================================================
	std::array<VkDescriptorSetLayoutBinding, 4> layoutBinding;
	layoutBinding[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);			// d(x,y,z) - aka - pingpong 0
	layoutBinding[1] = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);			// pingpong1
	layoutBinding[2] = vkUtility->InitLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);			// displacement	-output
	layoutBinding[3] = vkUtility->InitLayoutBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);			// ubo

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBinding.size());
	layoutInfo.pBindings = layoutBinding.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_fftDisplacement.descriptors.layout));

	// ================================================ descriptor sets =====================================================================
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_fftDisplacement.descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_fftDisplacement.descriptors.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->GetDevice(), &allocInfo, &m_fftDisplacement.descriptors.set));

	std::array<VkDescriptorBufferInfo, 3> bufferInfo;
	bufferInfo[0] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(ssbo_dxyz.block_id), ssbo_dxyz.offset, ssbo_dxyz.size);
	bufferInfo[1] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(ssbo_pingpong.block_id), ssbo_pingpong.offset, ssbo_pingpong.size);
	bufferInfo[2] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(ubo_displacement.block_id), ubo_displacement.offset, ubo_displacement.size);
	std::array<VkDescriptorImageInfo, 1> imageInfo;
	imageInfo[0] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_GENERAL, m_images.displacement.imageView, m_images.displacement.texSampler);		// displacement

	std::array<VkWriteDescriptorSet, 4> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_fftDisplacement.descriptors.set, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo[0]);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(m_fftDisplacement.descriptors.set, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferInfo[1]);
	writeDescrSet[2] = vkUtility->InitDescriptorSet(m_fftDisplacement.descriptors.set, 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageInfo[0]);
	writeDescrSet[3] = vkUtility->InitDescriptorSet(m_fftDisplacement.descriptors.set, 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo[2]);

	vkUpdateDescriptorSets(p_vkEngine->GetDevice(), static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);

	delete vkUtility;
}

void vkFFT::PrepareNormalDescriptorSets()
{
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

	std::array<VkDescriptorPoolSize, 2> descrPoolSize = {};
	descrPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descrPoolSize[0].descriptorCount = 1;
	descrPoolSize[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	descrPoolSize[1].descriptorCount = 2;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(descrPoolSize.size());
	createInfo.pPoolSizes = descrPoolSize.data();
	createInfo.maxSets = 1;

	VK_CHECK_RESULT(vkCreateDescriptorPool(p_vkEngine->GetDevice(), &createInfo, nullptr, &m_fftNormal.descriptors.pool));

	std::array<VkDescriptorSetLayoutBinding, 3> layoutBinding;
	layoutBinding[0] = vkUtility->InitLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);				// bindings for the UBO	 
	layoutBinding[1] = vkUtility->InitLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);				// bindings for the displacement map 
	layoutBinding[2] = vkUtility->InitLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);				// bindings for the gradient map 

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(layoutBinding.size());
	layoutInfo.pBindings = layoutBinding.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_fftNormal.descriptors.layout));

	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_fftNormal.descriptors.pool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_fftNormal.descriptors.layout;

	VK_CHECK_RESULT(vkAllocateDescriptorSets(p_vkEngine->GetDevice(), &allocInfo, &m_fftNormal.descriptors.set));

	std::array<VkDescriptorBufferInfo, 1> bufferInfo;
	bufferInfo[0] = vkUtility->InitBufferInfoDescriptor(p_vkMemory->blockBuffer(ubo_normal.block_id), ubo_normal.offset, ubo_normal.size);								// ubo
	std::array<VkDescriptorImageInfo, 2> imageInfo;
	imageInfo[0] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_GENERAL, m_images.displacement.imageView, m_images.displacement.texSampler);		// displacement map texture sampler
	imageInfo[1] = vkUtility->InitImageInfoDescriptor(VK_IMAGE_LAYOUT_GENERAL, m_images.normal.imageView, m_images.normal.texSampler);					// gradient sampler - output

	std::array<VkWriteDescriptorSet, 3> writeDescrSet = {};
	writeDescrSet[0] = vkUtility->InitDescriptorSet(m_fftNormal.descriptors.set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo[0]);
	writeDescrSet[1] = vkUtility->InitDescriptorSet(m_fftNormal.descriptors.set, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageInfo[0]);
	writeDescrSet[2] = vkUtility->InitDescriptorSet(m_fftNormal.descriptors.set, 2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageInfo[1]);

	vkUpdateDescriptorSets(p_vkEngine->GetDevice(), static_cast<uint32_t>(writeDescrSet.size()), writeDescrSet.data(), 0, nullptr);

	delete vkUtility;
}

void vkFFT::PreparePipelines()
{
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

	VkPipelineLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutInfo.setLayoutCount = 1;
	layoutInfo.pSetLayouts = &m_fftSpectrum.descriptors.layout;
	layoutInfo.pPushConstantRanges = 0;
	layoutInfo.pushConstantRangeCount = 0;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_fftSpectrum.pipelineInfo.layout));

	m_fftSpectrum.shader = vkUtility->InitShaders("terrain/water/fft/fft_spectrum-comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);

	VkComputePipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	createInfo.layout = m_fftSpectrum.pipelineInfo.layout;
	createInfo.stage = m_fftSpectrum.shader;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;

	// spectrum (h0(k) and h0(-k) pipeline
	VK_CHECK_RESULT(vkCreateComputePipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_fftSpectrum.pipelineInfo.pipeline));

	// butterfly pipeline
	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(uint32_t);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	layoutInfo.pPushConstantRanges = &pushConstant;
	layoutInfo.pushConstantRangeCount = 1;
	layoutInfo.pSetLayouts = &m_fftButterfly.descriptors.layout;
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_fftButterfly.pipelineInfo.layout));

	m_fftButterfly.shader = vkUtility->InitShaders("terrain/water/fft/fft_butterfly-comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
	createInfo.layout = m_fftButterfly.pipelineInfo.layout;
	createInfo.stage = m_fftButterfly.shader;
	VK_CHECK_RESULT(vkCreateComputePipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_fftButterfly.pipelineInfo.pipeline));

	// fft main pipeline
	pushConstant.size = sizeof(FFTPushData);
	layoutInfo.pSetLayouts = &m_fft.descriptors.layout;
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_fft.pipelineInfo.layout));

	m_fft.shader = vkUtility->InitShaders("terrain/water/fft/fft-comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
	createInfo.layout = m_fft.pipelineInfo.layout;
	createInfo.stage = m_fft.shader;
	VK_CHECK_RESULT(vkCreateComputePipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_fft.pipelineInfo.pipeline));

	// displacement pipeline
	pushConstant.size = sizeof(uint32_t);
	layoutInfo.pSetLayouts = &m_fftDisplacement.descriptors.layout;
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_fftDisplacement.pipelineInfo.layout));

	m_fftDisplacement.shader = vkUtility->InitShaders("terrain/water/fft/fft_displacement-comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
	createInfo.layout = m_fftDisplacement.pipelineInfo.layout;
	createInfo.stage = m_fftDisplacement.shader;
	VK_CHECK_RESULT(vkCreateComputePipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_fftDisplacement.pipelineInfo.pipeline));

	// normal compute pipeline
	layoutInfo.pPushConstantRanges = 0;
	layoutInfo.pushConstantRangeCount = 0;
	layoutInfo.pSetLayouts = &m_fftNormal.descriptors.layout;
	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &layoutInfo, nullptr, &m_fftNormal.pipelineInfo.layout));

	m_fftNormal.shader = vkUtility->InitShaders("terrain/water/fft/fft_normal-comp.spv", VK_SHADER_STAGE_COMPUTE_BIT);
	createInfo.layout = m_fftNormal.pipelineInfo.layout;
	createInfo.stage = m_fftNormal.shader;
	VK_CHECK_RESULT(vkCreateComputePipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_fftNormal.pipelineInfo.pipeline));

	delete vkUtility;
}

// ================================================= Heightmap functons - h(0)  ==========================================================================================================

float vkFFT::GeneratePhillips(glm::vec2 k, float windSpeed, float amplitude, glm::vec2 windDir)
{
	float k_len = glm::length(k);

	// calculate L
	float w = windSpeed;
	float L = w * w / GRAVITY;
	float damp = 1 / 1000;			// dampening of waves with small length

	windDir = glm::normalize(windDir);
	float amp = amplitude; //* 1e-7f;

	float k_sqr = k.x * k.x + k.y * k.y;
	float k_cos = k.x * windDir.x + k.y * windDir.y;

	float phil = amp * expf(-1 / (L * L * k_sqr)) / (k_sqr * k_sqr * k_sqr) * (k_cos * k_cos);
	if (k_cos < 0) {
		phil = 0.07f;
	}

	return phil * expf(-k_sqr * damp * damp);
}

void vkFFT::GenerateHeightMap(uint32_t patchLength, float windSpeed, float amplitude, glm::vec2 windDir)
{
	glm::vec2 k;
	RandomNumber *p_rand = new RandomNumber();
	uint32_t vecSize = N * N;

	std::vector<glm::vec2> h0k;
	h0k.resize(vecSize);

	std::vector<glm::vec2> h0minusk;
	h0minusk.resize(vecSize);

	for (uint32_t y = 0; y < N; ++y) {

		k.y = (-N / 2.0f + y) * (2.0f * PI / patchLength);

		for (uint32_t x = 0; x < N; ++x) {

			k.x = (-N / 2.0f + x) * (2.0f * PI / patchLength);

			// h0(k)
			float phillips = (k.x == 0.0f && k.y == 0.0f) ? 0.0f : sqrt(GeneratePhillips(k, windSpeed, amplitude, windDir));

			h0k[y * N + x].x = phillips * p_rand->GaussRandomNumber(RAND_MAX) * 0.7071068f;
			h0k[y * N + x].y = phillips * p_rand->GaussRandomNumber(RAND_MAX) * 0.7071068f;

			// h0(-k)
			float minus_phil = (k.x == 0.0f && k.y == 0.0f) ? 0.0f : sqrt(GeneratePhillips(-k, windSpeed, amplitude, windDir));

			h0minusk[y * N + x].x = minus_phil * p_rand->GaussRandomNumber(RAND_MAX) * 0.7071068f;
			h0minusk[y * N + x].y = minus_phil * p_rand->GaussRandomNumber(RAND_MAX) * 0.7071068f;

		}
	}
	delete p_rand;

	// upload omega and height map to gpu - ssbo used for compute shader input
	h0k_map = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, vecSize * sizeof(glm::vec2));
	p_vkMemory->MapDataToSegment<glm::vec2>(h0k_map, h0k);

	h0minusk_map = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, vecSize * sizeof(glm::vec2));
	p_vkMemory->MapDataToSegment<glm::vec2>(h0minusk_map, h0minusk);
}

void vkFFT::GenerateBitReversed()
{
	uint32_t log2N = log(N) / log(2);

	std::vector<uint32_t> bitreversed(N);

	for (uint32_t i = 0; i < N; ++i) {

		uint32_t rev = 0;
		uint32_t mod = i;

		for (uint32_t c = 0; c < log2N; ++c) {

			rev = (rev << 1) + (mod & 1);
			mod >>= 1;
		}

		bitreversed[i] = rev;
	}

	// map to local memory as this data won't be altered
	bitrev_map = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_STATIC, N * sizeof(uint32_t));
	p_vkMemory->MapDataToSegment<uint32_t>(bitrev_map, bitreversed);
}

void vkFFT::Update(float acc_time, uint32_t patchLength)
{
	// update the time dependent h0(t) buffer
	std::vector<SpecUboBuffer> ubo(1);
	ubo[0].L = static_cast<float>(patchLength);
	ubo[0].N = static_cast<float>(N);
	ubo[0].time = acc_time * 0.5;
	ubo[0].offset_dx = OFFSET_DX;
	ubo[0].offset_dy = OFFSET_DY;
	ubo[0].offset_dz = OFFSET_DZ;

	p_vkMemory->MapDataToSegment<SpecUboBuffer>(ubo_spectrum, ubo);
}

void vkFFT::Init(uint32_t patchLength, float windSpeed, float amplitude, glm::vec2 windDir)
{
	// generate LUT buffers - h0(k), h0(-k) and bit reversed indices
	GenerateHeightMap(patchLength, windSpeed, amplitude, windDir);
	GenerateBitReversed();

	// setup all the buffers needed for each pipeline 
	CreateBuffers();

	// prepare descriptors for each compute
	PrepareSpecDescriptorSets();
	PrepareButterflyDescriptorSets();
	PrepareFFTDescriptorSets();
	PrepareDisplacementDescriptorSets();
	PrepareNormalDescriptorSets();

	// create pipelines
	PreparePipelines();

	// generate cmd buffers for each pipeline - the twiddle bufefr is submitted immediately as this will be used as a LUT
	GenerateButterflyCmdBuffer();

	// fence required to ensure compute shader have finished before being used by the offscreen 
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	VK_CHECK_RESULT(vkCreateFence(p_vkEngine->GetDevice(), &fenceInfo, nullptr, &m_computeFence));

	// the same coomand buffer for each shader with memory barriers to ensure each has finished befoire the next starts reading data from the buffers
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);
	m_computeCmdBuffer = vkUtility->CreateCmdBuffer(VulkanUtility::VK_PRIMARY, VulkanUtility::VK_MULTI_USE, VK_NULL_HANDLE, VK_NULL_HANDLE, p_vkEngine->GetComputeCmdPool());

	GenerateSpectrumCmdBuffer(m_computeCmdBuffer);
	GenerateFFTCmdBuffer(m_computeCmdBuffer);
	GeneratDisplacementCmdBuffer(m_computeCmdBuffer);
	GenerateNormalCmdBuffer(m_computeCmdBuffer);

	vkEndCommandBuffer(m_computeCmdBuffer);

	delete vkUtility;
}
