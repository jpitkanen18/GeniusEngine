#pragma once
// Vulkan Texture Implementation (stub)

#include "../Texture.hpp"
#include "VulkanState.hpp"

namespace GE::Graphics::Vulkan
{

	class VulkanTexture : public ITexture
	{
	public:
		VulkanTexture() = default;
		~VulkanTexture() override;

		bool loadFromFile(const std::string &path) override;
		bool create(int width, int height, TextureFormat format, const void *data = nullptr) override;
		void bind(uint32_t slot = 0) override;
		void unbind() override;
		void setFilter(TextureFilter min, TextureFilter mag) override;
		void setWrap(TextureWrap s, TextureWrap t) override;
		int getWidth() const override { return this->m_width; }
		int getHeight() const override { return this->m_height; }

	private:
		VkImage m_image = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		VkImageView m_imageView = VK_NULL_HANDLE;
		VkSampler m_sampler = VK_NULL_HANDLE;
		int m_width = 0;
		int m_height = 0;
	};

} // namespace GE::Graphics::Vulkan
