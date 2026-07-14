#include "GLTexture.hpp"

// Simple stb_image-like loading (we'll generate procedural textures for the demo)
// For a full engine you'd include stb_image here

namespace GE::Graphics::GL
{

	GLTexture::~GLTexture()
	{
		if (this->m_texture)
			glDeleteTextures(1, &this->m_texture);
	}

	GLenum GLTexture::toGLFormat(TextureFormat fmt)
	{
		switch (fmt)
		{
		case TextureFormat::RGB8:
			return GL_RGB;
		case TextureFormat::RGBA8:
			return GL_RGBA;
		case TextureFormat::Depth24:
			return GL_DEPTH_COMPONENT;
		case TextureFormat::DepthStencil:
			return GL_DEPTH_STENCIL;
		}
		return GL_RGBA;
	}

	GLenum GLTexture::toGLFilter(TextureFilter f)
	{
		switch (f)
		{
		case TextureFilter::Nearest:
			return GL_NEAREST;
		case TextureFilter::Linear:
			return GL_LINEAR;
		case TextureFilter::LinearMipmap:
			return GL_LINEAR_MIPMAP_LINEAR;
		}
		return GL_LINEAR;
	}

	GLenum GLTexture::toGLWrap(TextureWrap w)
	{
		switch (w)
		{
		case TextureWrap::Repeat:
			return GL_REPEAT;
		case TextureWrap::Clamp:
			return GL_CLAMP_TO_EDGE;
		case TextureWrap::Mirror:
			return GL_MIRRORED_REPEAT;
		}
		return GL_REPEAT;
	}

	bool GLTexture::loadFromFile(const std::string & /*path*/)
	{
		// Stub: In production, use stb_image or similar
		// For now, create a 1x1 white texture as placeholder
		unsigned char white[] = {255, 255, 255, 255};
		return create(1, 1, TextureFormat::RGBA8, white);
	}

	bool GLTexture::create(int width, int height, TextureFormat format, const void *data)
	{
		this->m_width = width;
		this->m_height = height;

		glGenTextures(1, &this->m_texture);
		glBindTexture(GL_TEXTURE_2D, this->m_texture);

		GLenum glFmt = toGLFormat(format);
		GLenum internalFmt = (format == TextureFormat::RGBA8) ? GL_RGBA8 : GL_RGB8;
		GLenum type = GL_UNSIGNED_BYTE;

		if (format == TextureFormat::Depth24)
		{
			internalFmt = GL_DEPTH_COMPONENT24;
			type = GL_FLOAT;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, width, height, 0, glFmt, type, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		if (data)
			glGenerateMipmap(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

	void GLTexture::bind(uint32_t slot)
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, this->m_texture);
	}

	void GLTexture::unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void GLTexture::setFilter(TextureFilter min, TextureFilter mag)
	{
		glBindTexture(GL_TEXTURE_2D, this->m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, toGLFilter(min));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, toGLFilter(mag));
	}

	void GLTexture::setWrap(TextureWrap s, TextureWrap t)
	{
		glBindTexture(GL_TEXTURE_2D, this->m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, toGLWrap(s));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, toGLWrap(t));
	}

} // namespace GE::Graphics::GL
