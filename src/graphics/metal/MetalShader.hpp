#pragma once
// Metal Shader Implementation
// Compiles Metal Shading Language, manages pipeline state

#include "../Shader.hpp"

#ifdef __APPLE__

namespace GE::Graphics::Metal
{

	class MetalShader : public IShader
	{
	public:
		MetalShader() = default;
		~MetalShader() override;

		// For Metal, vertexSrc/fragmentSrc are MSL source strings
		// (or we can cross-compile from GLSL via SPIRV-Cross in the future)
		bool loadFromSource(const std::string &vertexSrc, const std::string &fragmentSrc) override;
		bool loadFromFile(const std::string &vertexPath, const std::string &fragmentPath) override;
		void bind() override;
		void unbind() override;

		void setInt(const std::string &name, int value) override;
		void setFloat(const std::string &name, float value) override;
		void setVec2(const std::string &name, const Vec2 &value) override;
		void setVec3(const std::string &name, const Vec3 &value) override;
		void setVec4(const std::string &name, const Vec4 &value) override;
		void setMat3(const std::string &name, const Mat3 &value) override;
		void setMat4(const std::string &name, const Mat4 &value) override;

		void *getPipelineState() { return this->m_pipelineState; }
		void *getDepthStencilState() { return this->m_depthStencilState; }

		// Uniform buffer management
		void *getUniformBuffer() { return this->m_uniformBuffer; }
		void uploadUniforms();

		void setDevice(void *device) { this->m_device = device; }
		void setEncoder(void *encoder) { this->m_encoder = encoder; }

	private:
		void *m_device = nullptr;
		void *m_encoder = nullptr;
		void *m_pipelineState = nullptr;
		void *m_depthStencilState = nullptr;
		void *m_vertexFunction = nullptr;
		void *m_fragmentFunction = nullptr;
		void *m_library = nullptr;
		void *m_uniformBuffer = nullptr;

		// CPU-side uniform storage
		// All fields use float4/mat4 to guarantee identical layout with MSL
		struct alignas(16) Uniforms
		{
			Mat4 viewProjection{1.0f}; // offset 0,   size 64
			Mat4 model{1.0f};		   // offset 64,  size 64
			// normalMatrix as 3×float4 to match Metal's float3x3 layout
			Vec4 normalCol0{1, 0, 0, 0};	 // offset 128, size 16
			Vec4 normalCol1{0, 1, 0, 0};	 // offset 144, size 16
			Vec4 normalCol2{0, 0, 1, 0};	 // offset 160, size 16
			Vec4 lightColor{1.0f};			 // offset 176, size 16
			Vec4 lightPos{0.0f};			 // offset 192, size 16 (xyz used)
			Vec4 viewPos{0.0f};				 // offset 208, size 16 (xyz used)
			Vec4 materialAmbient{0.1f};		 // offset 224, size 16
			Vec4 materialDiffuse{0.8f};		 // offset 240, size 16
			Vec4 materialSpecular{1.0f};	 // offset 256, size 16
			Vec4 materialEmissive{0.0f};	 // offset 272, size 16
			float materialShininess = 32.0f; // offset 288, size 4
			float _pad3[3] = {};			 // offset 292, size 12
		};
		Uniforms m_uniforms;
		bool m_uniformsDirty = true;

		bool createPipelineState();
	};

} // namespace GE::Graphics::Metal

#endif // __APPLE__
