#include "GLShader.hpp"
#include <fstream>
#include <sstream>

namespace GE::Graphics::GL
{

	GLShader::~GLShader()
	{
		if (this->m_program)
		{
			glDeleteProgram(this->m_program);
		}
	}

	bool GLShader::compileShader(GLuint shader, const std::string &source)
	{
		const char *src = source.c_str();
		glShaderSource(shader, 1, &src, nullptr);
		glCompileShader(shader);

		GLint success;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			char log[512];
			glGetShaderInfoLog(shader, 512, nullptr, log);
			std::cerr << "[GLShader] Compilation error:\n"
					  << log << "\n";
			return false;
		}
		return true;
	}

	bool GLShader::loadFromSource(const std::string &vertexSrc, const std::string &fragmentSrc)
	{
		GLuint vert = glCreateShader(GL_VERTEX_SHADER);
		GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);

		if (!compileShader(vert, vertexSrc) || !compileShader(frag, fragmentSrc))
		{
			glDeleteShader(vert);
			glDeleteShader(frag);
			return false;
		}

		this->m_program = glCreateProgram();
		glAttachShader(this->m_program, vert);
		glAttachShader(this->m_program, frag);
		glLinkProgram(this->m_program);

		GLint success;
		glGetProgramiv(this->m_program, GL_LINK_STATUS, &success);
		if (!success)
		{
			char log[512];
			glGetProgramInfoLog(this->m_program, 512, nullptr, log);
			std::cerr << "[GLShader] Link error:\n"
					  << log << "\n";
			glDeleteProgram(this->m_program);
			this->m_program = 0;
		}

		glDeleteShader(vert);
		glDeleteShader(frag);
		return this->m_program != 0;
	}

	bool GLShader::loadFromFile(const std::string &vertexPath, const std::string &fragmentPath)
	{
		auto readFile = [](const std::string &path) -> std::string
		{
			std::ifstream file(path);
			if (!file.is_open())
			{
				std::cerr << "[GLShader] Cannot open: " << path << "\n";
				return "";
			}
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

	void GLShader::bind() { glUseProgram(this->m_program); }
	void GLShader::unbind() { glUseProgram(0); }

	GLint GLShader::getUniformLocation(const std::string &name)
	{
		return glGetUniformLocation(this->m_program, name.c_str());
	}

	void GLShader::setInt(const std::string &name, int value)
	{
		glUniform1i(getUniformLocation(name), value);
	}
	void GLShader::setFloat(const std::string &name, float value)
	{
		glUniform1f(getUniformLocation(name), value);
	}
	void GLShader::setVec2(const std::string &name, const Vec2 &value)
	{
		glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
	}
	void GLShader::setVec3(const std::string &name, const Vec3 &value)
	{
		glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
	}
	void GLShader::setVec4(const std::string &name, const Vec4 &value)
	{
		glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
	}
	void GLShader::setMat3(const std::string &name, const Mat3 &value)
	{
		glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
	}
	void GLShader::setMat4(const std::string &name, const Mat4 &value)
	{
		glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
	}

} // namespace GE::Graphics::GL
