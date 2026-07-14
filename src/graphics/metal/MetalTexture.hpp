#pragma once
// Metal Texture Implementation

#include "../Texture.hpp"

#ifdef __APPLE__

namespace GE::Graphics::Metal
{

	class MetalTexture : public ITexture
	{
	public:
		MetalTexture() = default;
		~MetalTexture() override;

		bool loadFromFile(const std::string &path) override;
		bool create(int width, int height, TextureFormat format, const void *data = nullptr) override;
		void bind(uint32_t slot = 0) override;
		void unbind() override;
		void setFilter(TextureFilter min, TextureFilter mag) override;
		void setWrap(TextureWrap s, TextureWrap t) override;
		int getWidth() const override { return this->m_width; }
		int getHeight() const override { return this->m_height; }

		void setDevice(void *device) { this->m_device = device; }
		void setEncoder(void *encoder) { this->m_encoder = encoder; }

	private:
		void *m_device = nullptr;
		void *m_encoder = nullptr;
		void *m_texture = nullptr;
		void *m_sampler = nullptr;
		int m_width = 0;
		int m_height = 0;

		void createSampler(TextureFilter minFilter, TextureFilter magFilter,
						   TextureWrap wrapS, TextureWrap wrapT);
	};

} // namespace GE::Graphics::Metal

#endif // __APPLE__
