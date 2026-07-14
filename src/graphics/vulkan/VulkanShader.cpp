#include "VulkanShader.hpp"
#include <shaderc/shaderc.hpp>
#include <fstream>
#include <sstream>
#include <cstring>

namespace GE::Graphics::Vulkan
{

	VulkanShader::~VulkanShader()
	{
		VkDevice device = VulkanState::getDevice();
		if (device == VK_NULL_HANDLE)
			return;
		vkDeviceWaitIdle(device);
		if (this->m_pipeline)
			vkDestroyPipeline(device, this->m_pipeline, nullptr);
		if (this->m_linePipeline)
			vkDestroyPipeline(device, this->m_linePipeline, nullptr);
		if (this->m_pipelineLayout)
			vkDestroyPipelineLayout(device, this->m_pipelineLayout, nullptr);
		if (this->m_vertModule)
			vkDestroyShaderModule(device, this->m_vertModule, nullptr);
		if (this->m_fragModule)
			vkDestroyShaderModule(device, this->m_fragModule, nullptr);
	}

	std::vector<uint32_t> VulkanShader::compileGLSL(const std::string &source, bool isVertex)
	{
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;
		options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		auto kind = isVertex ? shaderc_vertex_shader : shaderc_fragment_shader;
		auto result = compiler.CompileGlslToSpv(source, kind, isVertex ? "vert" : "frag", options);

		if (result.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			std::cerr << "[VulkanShader] SPIR-V compilation failed:\n"
					  << result.GetErrorMessage() << "\n";
			return {};
		}
		return {result.cbegin(), result.cend()};
	}

	VkShaderModule VulkanShader::createShaderModule(const std::vector<uint32_t> &spirv)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = spirv.size() * sizeof(uint32_t);
		createInfo.pCode = spirv.data();

		VkShaderModule module;
		if (vkCreateShaderModule(VulkanState::getDevice(), &createInfo, nullptr, &module) != VK_SUCCESS)
		{
			std::cerr << "[VulkanShader] Failed to create shader module\n";
			return VK_NULL_HANDLE;
		}
		return module;
	}

	bool VulkanShader::loadFromSource(const std::string &vertexSrc, const std::string &fragmentSrc)
	{
		auto vertSpirv = compileGLSL(vertexSrc, true);
		auto fragSpirv = compileGLSL(fragmentSrc, false);
		if (vertSpirv.empty() || fragSpirv.empty())
			return false;

		this->m_vertModule = createShaderModule(vertSpirv);
		this->m_fragModule = createShaderModule(fragSpirv);
		if (!this->m_vertModule || !this->m_fragModule)
			return false;

		if (!createPipeline())
			return false;

		std::cout << "[VulkanShader] Pipeline created successfully\n";
		return true;
	}

	bool VulkanShader::loadFromFile(const std::string &vertexPath, const std::string &fragmentPath)
	{
		auto readFile = [](const std::string &path) -> std::string
		{
			std::ifstream file(path);
			if (!file.is_open())
				return "";
			std::stringstream ss;
			ss << file.rdbuf();
			return ss.str();
		};
		std::string vSrc = readFile(vertexPath);
		std::string fSrc = readFile(fragmentPath);
		if (vSrc.empty() || fSrc.empty())
			return false;
		return loadFromSource(vSrc, fSrc);
	}

	bool VulkanShader::createPipeline()
	{
		VkDevice device = VulkanState::getDevice();

		// Push constant range — full PushData visible to both stages
		VkPushConstantRange pushRange{};
		pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushRange.offset = 0;
		pushRange.size = sizeof(PushData);

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = 0;
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushRange;

		if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &this->m_pipelineLayout) != VK_SUCCESS)
			return false;

		// Shader stages
		VkPipelineShaderStageCreateInfo vertStage{};
		vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertStage.module = this->m_vertModule;
		vertStage.pName = "main";

		VkPipelineShaderStageCreateInfo fragStage{};
		fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragStage.module = this->m_fragModule;
		fragStage.pName = "main";

		VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

		// Vertex input
		VkVertexInputBindingDescription bindingDesc{};
		bindingDesc.binding = 0;
		bindingDesc.stride = sizeof(Vertex);
		bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription attrs[4] = {};
		attrs[0].location = 0;
		attrs[0].binding = 0;
		attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrs[0].offset = offsetof(Vertex, position);
		attrs[1].location = 1;
		attrs[1].binding = 0;
		attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attrs[1].offset = offsetof(Vertex, normal);
		attrs[2].location = 2;
		attrs[2].binding = 0;
		attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
		attrs[2].offset = offsetof(Vertex, texCoord);
		attrs[3].location = 3;
		attrs[3].binding = 0;
		attrs[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attrs[3].offset = offsetof(Vertex, color);

		VkPipelineVertexInputStateCreateInfo vertexInput{};
		vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInput.vertexBindingDescriptionCount = 1;
		vertexInput.pVertexBindingDescriptions = &bindingDesc;
		vertexInput.vertexAttributeDescriptionCount = 4;
		vertexInput.pVertexAttributeDescriptions = attrs;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		VkPipelineColorBlendAttachmentState colorBlend{};
		colorBlend.blendEnable = VK_TRUE;
		colorBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlend.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlend.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
									VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo blending{};
		blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		blending.attachmentCount = 1;
		blending.pAttachments = &colorBlend;

		VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynState{};
		dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynState.dynamicStateCount = 2;
		dynState.pDynamicStates = dynStates;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = stages;
		pipelineInfo.pVertexInputState = &vertexInput;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &blending;
		pipelineInfo.pDynamicState = &dynState;
		pipelineInfo.layout = this->m_pipelineLayout;
		pipelineInfo.renderPass = VulkanState::getRenderPass();
		pipelineInfo.subpass = 0;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->m_pipeline) != VK_SUCCESS)
		{
			std::cerr << "[VulkanShader] Failed to create triangle pipeline\n";
			return false;
		}

		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->m_linePipeline) != VK_SUCCESS)
		{
			std::cerr << "[VulkanShader] Failed to create line pipeline\n";
			return false;
		}

		return true;
	}

	void VulkanShader::bind()
	{
		VkCommandBuffer cmd = VulkanState::getCurrentCommandBuffer();
		if (cmd == VK_NULL_HANDLE || !this->m_pipeline)
			return;

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_pipeline);
	}

	void VulkanShader::bindForLines()
	{
		VkCommandBuffer cmd = VulkanState::getCurrentCommandBuffer();
		if (cmd == VK_NULL_HANDLE || !this->m_linePipeline)
			return;

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, this->m_linePipeline);
	}

	void VulkanShader::unbind() {}

	// Push current constants into the command buffer
	void VulkanShader::pushConstants()
	{
		VkCommandBuffer cmd = VulkanState::getCurrentCommandBuffer();
		if (cmd != VK_NULL_HANDLE && this->m_pipelineLayout)
			vkCmdPushConstants(cmd, this->m_pipelineLayout,
							   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
							   0, sizeof(PushData), &this->m_push);
	}

	void VulkanShader::setInt(const std::string &, int) {}
	void VulkanShader::setFloat(const std::string &name, float value)
	{
		if (name == "u_material.shininess" || name == "materialShininess")
			this->m_push.materialExtra.x = value;
		pushConstants();
	}

	void VulkanShader::setVec2(const std::string &, const Vec2 &) {}

	void VulkanShader::setVec3(const std::string &name, const Vec3 &value)
	{
		if (name == "u_lightPos")
			this->m_push.lightPos = Vec4(value, 0.0f);
		else if (name == "u_viewPos")
			this->m_push.viewPos = Vec4(value, 0.0f);
		pushConstants();
	}

	void VulkanShader::setVec4(const std::string &name, const Vec4 &value)
	{
		if (name == "u_lightColor")
			this->m_push.lightColor = value;
		else if (name == "u_material.ambient")
			this->m_push.materialAmbient = value;
		else if (name == "u_material.diffuse")
			this->m_push.materialDiffuse = value;
		else if (name == "u_material.specular")
			this->m_push.materialSpecular = value;
		else if (name == "u_material.emissive")
			this->m_push.materialEmissive = value;
		pushConstants();
	}

	void VulkanShader::setMat3(const std::string &name, const Mat3 &value)
	{
		if (name == "u_normalMatrix")
		{
			this->m_push.normalCol0 = Vec4(value[0], 0.0f);
			this->m_push.normalCol1 = Vec4(value[1], 0.0f);
			this->m_push.normalCol2 = Vec4(value[2], 0.0f);
		}
		pushConstants();
	}

	void VulkanShader::setMat4(const std::string &name, const Mat4 &value)
	{
		if (name == "u_viewProjection")
			this->m_push.viewProjection = value;
		else if (name == "u_model")
			this->m_push.model = value;
		pushConstants();
	}

} // namespace GE::Graphics::Vulkan
