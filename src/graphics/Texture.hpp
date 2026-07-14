#pragma once
// GeniusEngine Texture Interface

#include "../core/Types.hpp"

namespace GE::Graphics
{

	enum class TextureFormat
	{
		RGB8,
		RGBA8,
		Depth24,
		DepthStencil
	};

	enum class TextureFilter
	{
		Nearest,
		Linear,
		LinearMipmap
	};

	enum class TextureWrap
	{
		Repeat,
		Clamp,
		Mirror
	};

	class ITexture
	{
	public:
		virtual ~ITexture() = default;
		virtual bool loadFromFile(const std::string &path) = 0;
		virtual bool create(int width, int height, TextureFormat format, const void *data = nullptr) = 0;
		virtual void bind(uint32_t slot = 0) = 0;
		virtual void unbind() = 0;
		virtual void setFilter(TextureFilter min, TextureFilter mag) = 0;
		virtual void setWrap(TextureWrap s, TextureWrap t) = 0;
		virtual int getWidth() const = 0;
		virtual int getHeight() const = 0;
	};

} // namespace GE::Graphics
