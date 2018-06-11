#include "VulkanGUI.h"
#include "VulkanCore/VulkanEngine.h"
#include "VulkanCore/VkPostProcess.h"
#include "VulkanCore/VulkanWater.h"
#include "Systems/camera_system.h"
#include "Systems/input_system.h"
#include "Engine/World.h"
#include <algorithm>
#include <imgui.h>

#define NOMINMAX					// stops the annoying collision between windows max/min and std::max/min
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

VulkanGUI::VulkanGUI(VulkanEngine *engine) :
	p_vkEngine(engine)
{
}


VulkanGUI::~VulkanGUI()
{
	Destroy();
}

void VulkanGUI::SetupGUI(VkMemoryManager *p_vkMemory)
{
	
	// setup imGui to be used with GLFW and Vulkan
	ImGui::CreateContext();
	ImGuiIO& guiIO = ImGui::GetIO();

	// setup imgui with glfw window
	auto p_input = p_vkEngine->GetCurrentWorld()->RequestSystem<InputSystem>();
	guiIO.ImeWindowHandle = glfwGetWin32Window(p_input->CurrentGLFWwindow());
	
	// setup gui colour scheme
	//ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.6f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.0f, 0.0f, 0.4f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

	// io stuff
	guiIO.DisplaySize = ImVec2(p_vkEngine->GetSurfaceExtentW(), p_vkEngine->GetSurfaceExtentH());
	guiIO.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	//guiIO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;		// enable keyboard through ImGUI

	// Vulkan setup - image sampler for font
	unsigned char *fontData;
	int fontWidth, fontHeight;
	guiIO.Fonts->GetTexDataAsRGBA32(&fontData, &fontWidth, &fontHeight);		// convert to data we can use with vulkan - using default font

	// create image and upload font data 
	m_fontImage.PrepareImage(VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, fontWidth, fontHeight, p_vkEngine, 16.0f);
	m_fontImage.UploadDataToImage(fontData, fontWidth * fontHeight * 4 * sizeof(char), p_vkMemory, p_vkEngine);
}

void VulkanGUI::PrepareDescriptors()
{
	std::vector<VkDescriptors::LayoutBinding> layoutBind =
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }			// bindings for the font image sampler
	};
	m_descriptors.AddDescriptorBindings(layoutBind);

	std::vector<VkDescriptorImageInfo> imageInfo =
	{
		{ m_fontImage.texSampler, m_fontImage.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
	};

	m_descriptors.GenerateDescriptorSets(nullptr, imageInfo.data(), p_vkEngine->GetDevice());
}

void VulkanGUI::PreparePipeline()
{	
	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

	// create pipeline cache - not normally created but being used by ImGUI API
	VkPipelineCacheCreateInfo cacheInfo = {};
	cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	VK_CHECK_RESULT(vkCreatePipelineCache(p_vkEngine->GetDevice(), &cacheInfo, nullptr, &m_pipelineInfo.cache));
	
	// prepare vertex layout
	VkVertexInputBindingDescription bindDescr = {}; 
	std::array<VkVertexInputAttributeDescription, 3> attrDescr = {};
	PrepareGUIVertex(bindDescr, attrDescr);

	VkPipelineVertexInputStateCreateInfo vertexInfo = {};
	vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInfo.vertexBindingDescriptionCount = 1;
	vertexInfo.pVertexBindingDescriptions = &bindDescr;
	vertexInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescr.size());
	vertexInfo.pVertexAttributeDescriptions = attrDescr.data();

	VkPipelineInputAssemblyStateCreateInfo assemblyInfo = {};
	assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState = vkUtility->InitViewPortCreateInfo(p_vkEngine->GetViewPort(), p_vkEngine->GetScissor(), 1, 1);

	VkPipelineRasterizationStateCreateInfo rasterInfo = vkUtility->InitRasterzationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);

	VkPipelineMultisampleStateCreateInfo multiInfo = vkUtility->InitMultisampleState(VK_SAMPLE_COUNT_1_BIT);

	// colour attachment required for each colour buffer - transparency required for GUI
	std::array<VkPipelineColorBlendAttachmentState, 1> colorAttach = {};
	colorAttach[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttach[0].blendEnable = VK_TRUE;
	colorAttach[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorAttach[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorAttach[0].colorBlendOp = VK_BLEND_OP_ADD;
	colorAttach[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorAttach[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorAttach[0].alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorInfo = {};
	colorInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorInfo.logicOpEnable = VK_FALSE;
	colorInfo.attachmentCount = static_cast<uint32_t>(colorAttach.size());
	colorInfo.pAttachments = colorAttach.data();

	VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicInfo = {};
	dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicInfo.pDynamicStates = states;
	dynamicInfo.dynamicStateCount = 2;

	VkPipelineDepthStencilStateCreateInfo depthInfo = {};
	depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthInfo.depthTestEnable = VK_FALSE;
	depthInfo.depthWriteEnable = VK_FALSE;
	depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

	VkPushConstantRange pushConstant = {};
	pushConstant.size = sizeof(PushConstant);
	pushConstant.offset = 0;
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	VkPushConstantRange pushConstantArray[] = { pushConstant };

	VkPipelineLayoutCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineInfo.setLayoutCount = 1;
	pipelineInfo.pSetLayouts = &m_descriptors.layout;
	pipelineInfo.pPushConstantRanges = &pushConstant;
	pipelineInfo.pushConstantRangeCount = 1;

	VK_CHECK_RESULT(vkCreatePipelineLayout(p_vkEngine->GetDevice(), &pipelineInfo, nullptr, &m_pipelineInfo.layout));

	// sahders for rendering full screen quad
	m_shader[0] = vkUtility->InitShaders("GUI/gui-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
	m_shader[1] = vkUtility->InitShaders("GUI/gui-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.stageCount = 2;
	createInfo.pStages = m_shader.data();
	createInfo.pVertexInputState = &vertexInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pViewportState = &viewportState;
	createInfo.pRasterizationState = &rasterInfo;
	createInfo.pMultisampleState = &multiInfo;
	createInfo.pDepthStencilState = &depthInfo;
	createInfo.pColorBlendState = &colorInfo;
	createInfo.pDynamicState = &dynamicInfo;
	createInfo.layout = m_pipelineInfo.layout;
	createInfo.renderPass = p_vkEngine->GetFinalRenderPass();		// using the final renderpass - to surface presentation KHR
	createInfo.subpass = 0;
	createInfo.basePipelineIndex = -1;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(p_vkEngine->GetDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &m_pipelineInfo.pipeline));

	delete vkUtility;
}

void VulkanGUI::NewFrame()
{
	auto camera = p_vkEngine->GetCurrentWorld()->RequestSystem<CameraSystem>();

	Update();

	// generate new imGui frame using GLFW window
	ImGui::NewFrame();

	// Preferences window
	ImGui::TextUnformatted("OmegaEngine Preferences");

	glm::vec3 camera_pos = camera->GetCameraPosition();

	// info window - camera pos, fps, etc.
	ImGui::Text("Camera");
	ImGui::InputFloat3("position", &camera_pos.x, 2);

	// settings window
	ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("OmegaEngine Settings");

	// display wireframe/filled polygons
	if (ImGui::Checkbox("Draw Wireframe", &m_guiSettings.wireframe)) {
		p_vkEngine->ClearDrawState();		// regenerate command buffers 
	}

	ImGui::Checkbox("Post process", &m_guiSettings.showFog);
	
	if (ImGui::Checkbox("Lights On", &m_guiSettings.lights)) {
		p_vkEngine->ClearDrawState();
	}

	ImGui::Separator();

	// terrain type combo
	const char *items[] = { "Water", "Land" };
	if (ImGui::Combo("Terrain Type", &m_guiSettings.terrainType, items, IM_ARRAYSIZE(items))) {
		p_vkEngine->ClearDrawState();
	}

	// water attributes
	if (m_guiSettings.terrainType == 0) {

		if (ImGui::SliderFloat("Amplitude", &m_guiSettings.amplitude, 0.0f, 200.0f)) {
			p_vkEngine->VkModule<VulkanWater>()->GenerateH0Map();
		}
		if (ImGui::SliderFloat("Choppiness", &m_guiSettings.choppiness, 0.0f, 10.0f)) {
			p_vkEngine->ClearDrawState();
		}
		if (ImGui::SliderFloat("Tesselation", &m_guiSettings.tesselation, 0.0f, 2.0f)) {
			p_vkEngine->ClearDrawState();
		}
		if (ImGui::SliderFloat("Edge Factor", &m_guiSettings.edgeFactor, 0.0f, 100.0f)) {
			p_vkEngine->ClearDrawState();
		}
	}
	else {
		// terrain attributes
		if (ImGui::SliderFloat("Displacement", &m_guiSettings.displacement, 0.0f, 100.0f)) {
			p_vkEngine->ClearDrawState();
		}
		if (ImGui::SliderFloat("Tesselation", &m_guiSettings.tesselation, 0.0f, 2.0f)) {
			p_vkEngine->ClearDrawState();
		}
		if (ImGui::SliderFloat("Edge Factor", &m_guiSettings.edgeFactor, 0.0f, 100.0f)) {
			p_vkEngine->ClearDrawState();
		}

	}

	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(650, 200), ImGuiSetCond_FirstUseEver);
	//ImGui::ShowTestWindow();

	ImGui::Render();
}

void VulkanGUI::UpdateBuffers(VkMemoryManager *p_vkMemory)
{
	ImDrawData *guiDraw = ImGui::GetDrawData();

	uint32_t vertSize = guiDraw->TotalVtxCount * sizeof(ImDrawVert);
	uint32_t indSize = guiDraw->TotalIdxCount * sizeof(ImDrawIdx);

	// if buffers have changed in size since last update, destroy and create new buffers
	if (m_vertices.data == nullptr || m_vertCount != guiDraw->TotalVtxCount) {

		p_vkMemory->DestroySegment(m_vertices);
		m_vertices = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, vertSize);
		m_vertCount = guiDraw->TotalVtxCount;
	}

	if (m_indices.data == nullptr || m_indCount != guiDraw->TotalIdxCount) {

		p_vkMemory->DestroySegment(m_indices);
		m_indices = p_vkMemory->AllocateSegment(MemoryUsage::VK_BUFFER_DYNAMIC, indSize);
		m_indCount = guiDraw->TotalIdxCount;
	}

	// copy data to buffers
	uint32_t vertOffset = 0;
	uint32_t indOffset = 0;

	for (int c = 0; c < guiDraw->CmdListsCount; ++c) {

		const ImDrawList *list = guiDraw->CmdLists[c];
		p_vkMemory->MapDataToSegment(m_vertices, list->VtxBuffer.Data, list->VtxBuffer.Size * sizeof(ImDrawVert), vertOffset);
		p_vkMemory->MapDataToSegment(m_indices, list->IdxBuffer.Data, list->IdxBuffer.Size * sizeof(ImDrawIdx), indOffset);
		vertOffset += list->VtxBuffer.Size * sizeof(ImDrawVert);
		indOffset += list->IdxBuffer.Size * sizeof(ImDrawIdx);
	}
}

void VulkanGUI::GenerateCmdBuffer(VkCommandBuffer cmdBuffer, VkMemoryManager *p_vkMemory)
{
	ImGuiIO &guiIO = ImGui::GetIO();

	// create new GUI frame
	NewFrame();
	
	// check whether the buffers have chnaged in size
	UpdateBuffers(p_vkMemory);

	VulkanUtility *vkUtility = new VulkanUtility(p_vkEngine);

	VkViewport viewport = vkUtility->InitViewPort(guiIO.DisplaySize.x, guiIO.DisplaySize.y, 0.0f, 1.0f);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

	VkDeviceSize offsets[1]{ m_vertices.offset };

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.pipeline);

	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &p_vkMemory->blockBuffer(m_vertices.block_id), offsets);
	vkCmdBindIndexBuffer(cmdBuffer, p_vkMemory->blockBuffer(m_indices.block_id), m_indices.offset, VK_INDEX_TYPE_UINT16);
	
	vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineInfo.layout, 0, 1, &m_descriptors.set, 0, NULL);
	
	// push window scale constant to shader
	PushConstant push;
	push.scale = glm::vec2(2.0f / guiIO.DisplaySize.x, 2.0f / guiIO.DisplaySize.y);
	vkCmdPushConstants(cmdBuffer, m_pipelineInfo.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstant), &push);

	ImDrawData *guiDraw = ImGui::GetDrawData();
	uint32_t vertOffset = 0;
	uint32_t indOffset = 0;

	for (uint32_t c = 0; c < guiDraw->CmdListsCount; ++c) {
	
		const ImDrawList *list = guiDraw->CmdLists[c];
		
		for (uint32_t i = 0; i < list->CmdBuffer.Size; ++i) {

			const ImDrawCmd *cmd = &list->CmdBuffer[i];

			// calculate scissor according to GUI window size - for clipping purposes
			VkRect2D scissor = vkUtility->InitScissor(cmd->ClipRect.z - cmd->ClipRect.x, cmd->ClipRect.w - cmd->ClipRect.y, std::max(cmd->ClipRect.x, 0.0f), std::max(cmd->ClipRect.y, 0.0f));
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
			vkCmdDrawIndexed(cmdBuffer,cmd->ElemCount, 1, indOffset, vertOffset, 0);

			indOffset += cmd->ElemCount;
		}
		vertOffset += list->VtxBuffer.Size;
	}
	delete vkUtility;
}

void VulkanGUI::Update()
{
	auto p_input = p_vkEngine->GetCurrentWorld()->RequestSystem<InputSystem>();

	ImGuiIO& guiIO = ImGui::GetIO();

	// update mouse position using info from glfw
	double posX, posY;
	p_input->GetCursorPos(&posX, &posY);
	guiIO.MousePos = ImVec2((float)posX, (float)posY);

	// update mouse button state
	guiIO.MouseDown[0] = p_input->ButtonState(GLFW_MOUSE_BUTTON_LEFT);
	guiIO.MouseDown[1] = p_input->ButtonState(GLFW_MOUSE_BUTTON_RIGHT);
	
}

void VulkanGUI::Init(VkMemoryManager *p_vkMemory)
{
	SetupGUI(p_vkMemory);
	PrepareDescriptors();
	PreparePipeline();
}

void VulkanGUI::Destroy()
{
	// destroy imGUI
	ImGui::DestroyContext();

	// destroy vulkan related stuff
	m_fontImage.Destroy(p_vkEngine->GetDevice());
	vkDestroyPipelineLayout(p_vkEngine->GetDevice(), m_pipelineInfo.layout, nullptr);
	vkDestroyPipeline(p_vkEngine->GetDevice(), m_pipelineInfo.pipeline, nullptr);

	p_vkEngine = nullptr;
}

void VulkanGUI::PrepareGUIVertex(VkVertexInputBindingDescription& bindDescr, std::array<VkVertexInputAttributeDescription, 3>& attrDescr)
{
	
	bindDescr.binding = 0;
	bindDescr.stride = sizeof(ImDrawVert);
	bindDescr.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Vertex layout 0: pos
	attrDescr[0].binding = 0;
	attrDescr[0].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[0].location = 0;
	attrDescr[0].offset = offsetof(ImDrawVert, pos);

	// Vertex layout 1: uv
	attrDescr[1].binding = 0;
	attrDescr[1].format = VK_FORMAT_R32G32_SFLOAT;
	attrDescr[1].location = 1;
	attrDescr[1].offset = offsetof(ImDrawVert, uv);

	// Vertex layout 2: colour
	attrDescr[2].binding = 0;
	attrDescr[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	attrDescr[2].location = 2;
	attrDescr[2].offset = offsetof(ImDrawVert, col);
}

