#pragma once
// GeniusEngine Shader Interface

#include "../core/Types.hpp"

namespace GE::Graphics
{

	class IShader
	{
	public:
		virtual ~IShader() = default;
		virtual bool loadFromSource(const std::string &vertexSrc, const std::string &fragmentSrc) = 0;
		virtual bool loadFromFile(const std::string &vertexPath, const std::string &fragmentPath) = 0;
		virtual void bind() = 0;
		virtual void bindForLines() { bind(); } // Override for Vulkan line topology
		virtual void unbind() = 0;

		// Uniforms
		virtual void setInt(const std::string &name, int value) = 0;
		virtual void setFloat(const std::string &name, float value) = 0;
		virtual void setVec2(const std::string &name, const Vec2 &value) = 0;
		virtual void setVec3(const std::string &name, const Vec3 &value) = 0;
		virtual void setVec4(const std::string &name, const Vec4 &value) = 0;
		virtual void setMat3(const std::string &name, const Mat3 &value) = 0;
		virtual void setMat4(const std::string &name, const Mat4 &value) = 0;
	};

} // namespace GE::Graphics
