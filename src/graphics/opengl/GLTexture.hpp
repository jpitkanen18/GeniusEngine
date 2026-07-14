#pragma once
// OpenGL Texture Implementation

#include "../Texture.hpp"
#include <glad/gl.h>

namespace GE::Graphics::GL
{

	class GLTexture : public ITexture
	{
	public:
		GLTexture() = default;
		~GLTexture() override;

		bool loadFromFile(const std::string &path) override;
		bool create(int width, int height, TextureFormat format, const void *data = nullptr) override;
		void bind(uint32_t slot = 0) override;
		void unbind() override;
		void setFilter(TextureFilter min, TextureFilter mag) override;
		void setWrap(TextureWrap s, TextureWrap t) override;
		int getWidth() const override { return this->m_width; }
		int getHeight() const override { return this->m_height; }

		GLuint getHandle() const { return this->m_texture; }

	private:
		GLuint m_texture = 0;
		int m_width = 0;
		int m_height = 0;
		static GLenum toGLFormat(TextureFormat fmt);
		static GLenum toGLFilter(TextureFilter f);
		static GLenum toGLWrap(TextureWrap w);
	};

} // namespace GE::Graphics::GL
