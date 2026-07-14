#pragma once
// OpenGL Shader Implementation

#include "../Shader.hpp"
#include <glad/gl.h>

namespace GE::Graphics::GL
{

	class GLShader : public IShader
	{
	public:
		GLShader() = default;
		~GLShader() override;

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

		GLuint getProgram() const { return this->m_program; }

	private:
		GLuint m_program = 0;
		GLint getUniformLocation(const std::string &name);
		bool compileShader(GLuint shader, const std::string &source);
	};

} // namespace GE::Graphics::GL
