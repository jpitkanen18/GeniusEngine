#include "VulkanTexture.hpp"
#include <iostream>

namespace GE::Graphics::Vulkan
{

	VulkanTexture::~VulkanTexture()
	{
		VkDevice device = VulkanState::getDevice();
		if (device == VK_NULL_HANDLE)
			return;
		if (this->m_sampler)
			vkDestroySampler(device, this->m_sampler, nullptr);
		if (this->m_imageView)
			vkDestroyImageView(device, this->m_imageView, nullptr);
		if (this->m_image)
			vkDestroyImage(device, this->m_image, nullptr);
		if (this->m_memory)
			vkFreeMemory(device, this->m_memory, nullptr);
	}

	bool VulkanTexture::loadFromFile(const std::string &)
	{
		std::cerr << "[VulkanTexture] loadFromFile not yet implemented\n";
		return false;
	}

	bool VulkanTexture::create(int width, int height, TextureFormat, const void *)
	{
		this->m_width = width;
		this->m_height = height;
		// Stub — texture creation requires descriptor sets + sampler setup
		return true;
	}

	void VulkanTexture::bind(uint32_t) {}
	void VulkanTexture::unbind() {}
	void VulkanTexture::setFilter(TextureFilter, TextureFilter) {}
	void VulkanTexture::setWrap(TextureWrap, TextureWrap) {}

} // namespace GE::Graphics::Vulkan
