#pragma once
#include "../Shader.hpp"
#include "VulkanState.hpp"

namespace GE::Graphics::Vulkan
{

	class VulkanShader : public IShader
	{
	public:
		VulkanShader() = default;
		~VulkanShader() override;

		bool loadFromSource(const std::string &vertexSrc, const std::string &fragmentSrc) override;
		bool loadFromFile(const std::string &vertexPath, const std::string &fragmentPath) override;
		void bind() override;
		void bindForLines() override;
		void unbind() override;

		void setInt(const std::string &name, int value) override;
		void setFloat(const std::string &name, float value) override;
		void setVec2(const std::string &name, const Vec2 &value) override;
		void setVec3(const std::string &name, const Vec3 &value) override;
		void setVec4(const std::string &name, const Vec4 &value) override;
		void setMat3(const std::string &name, const Mat3 &value) override;
		void setMat4(const std::string &name, const Mat4 &value) override;

	private:
		VkPipeline m_pipeline = VK_NULL_HANDLE;
		VkPipeline m_linePipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
		VkShaderModule m_vertModule = VK_NULL_HANDLE;
		VkShaderModule m_fragModule = VK_NULL_HANDLE;

		// Push constant data — written into the command buffer per draw call.
		// Unlike UBO, each draw gets its own copy of the data.
		struct PushData
		{
			Mat4 viewProjection{1.0f};					   // 0
			Mat4 model{1.0f};							   // 64
			Vec4 normalCol0{1, 0, 0, 0};				   // 128
			Vec4 normalCol1{0, 1, 0, 0};				   // 144
			Vec4 normalCol2{0, 0, 1, 0};				   // 160
			Vec4 lightColor{1, 1, 1, 1};				   // 176
			Vec4 lightPos{0, 10, 0, 0};					   // 192
			Vec4 viewPos{0, 0, 0, 0};					   // 208
			Vec4 materialAmbient{0.2f, 0.2f, 0.2f, 1.0f};  // 224
			Vec4 materialDiffuse{0.8f, 0.8f, 0.8f, 1.0f};  // 240
			Vec4 materialSpecular{0.5f, 0.5f, 0.5f, 1.0f}; // 256
			Vec4 materialEmissive{0, 0, 0, 0};			   // 272
			Vec4 materialExtra{32.0f, 0, 0, 0};			   // 288 (.x = shininess)
		}; // total = 304 bytes
		static_assert(sizeof(PushData) <= 4096, "Push constants too large");
		PushData m_push;

		std::vector<uint32_t> compileGLSL(const std::string &source, bool isVertex);
		VkShaderModule createShaderModule(const std::vector<uint32_t> &spirv);
		bool createPipeline();
		void pushConstants();
	};

} // namespace GE::Graphics::Vulkan
