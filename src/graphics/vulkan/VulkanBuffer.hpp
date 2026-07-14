#pragma once
// Vulkan Buffer Implementation

#include "../Buffer.hpp"
#include "VulkanState.hpp"

namespace GE::Graphics::Vulkan
{

	class VulkanVertexBuffer : public IVertexBuffer
	{
	public:
		VulkanVertexBuffer() = default;
		~VulkanVertexBuffer() override;

		void create(const void *data, size_t size, BufferUsage usage) override;
		void update(const void *data, size_t size, size_t offset = 0) override;
		void bind() override;
		void unbind() override;

		VkBuffer getBuffer() const { return this->m_buffer; }

	private:
		VkBuffer m_buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		size_t m_size = 0;
		void *m_mapped = nullptr;
		bool m_hostVisible = false;

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
						  VkMemoryPropertyFlags properties,
						  VkBuffer &buffer, VkDeviceMemory &memory);
	};

	class VulkanIndexBuffer : public IIndexBuffer
	{
	public:
		VulkanIndexBuffer() = default;
		~VulkanIndexBuffer() override;

		void create(const uint32_t *data, size_t count, BufferUsage usage) override;
		void bind() override;
		void unbind() override;
		size_t getCount() const override { return this->m_count; }

		VkBuffer getBuffer() const { return this->m_buffer; }

	private:
		VkBuffer m_buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_memory = VK_NULL_HANDLE;
		size_t m_count = 0;
		void *m_mapped = nullptr;
	};

	class VulkanVertexArray : public IVertexArray
	{
	public:
		VulkanVertexArray() = default;
		~VulkanVertexArray() override = default;

		void create() override;
		void bind() override;
		void unbind() override;
		void addVertexBuffer(std::shared_ptr<IVertexBuffer> vbo) override;
		void setIndexBuffer(std::shared_ptr<IIndexBuffer> ibo) override;
		void setupVertexLayout() override;
		void draw(PrimitiveType primitive, size_t count) override;
		void drawIndexed(PrimitiveType primitive) override;

	private:
		std::shared_ptr<IVertexBuffer> m_vbo;
		std::shared_ptr<IIndexBuffer> m_ibo;
	};

} // namespace GE::Graphics::Vulkan
