#include "VulkanBuffer.hpp"
#include <cstring>

namespace GE::Graphics::Vulkan
{

	// --- Vertex Buffer ---

	VulkanVertexBuffer::~VulkanVertexBuffer()
	{
		VkDevice device = VulkanState::getDevice();
		if (device == VK_NULL_HANDLE)
			return;
		if (this->m_mapped)
			vkUnmapMemory(device, this->m_memory);
		if (this->m_buffer)
			vkDestroyBuffer(device, this->m_buffer, nullptr);
		if (this->m_memory)
			vkFreeMemory(device, this->m_memory, nullptr);
	}

	void VulkanVertexBuffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
										  VkMemoryPropertyFlags properties,
										  VkBuffer &buffer, VkDeviceMemory &memory)
	{
		VkDevice device = VulkanState::getDevice();

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device, buffer, &memReqs);

		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(VulkanState::getPhysicalDevice(), &memProps);

		uint32_t memTypeIdx = 0;
		for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
		{
			if ((memReqs.memoryTypeBits & (1 << i)) &&
				(memProps.memoryTypes[i].propertyFlags & properties) == properties)
			{
				memTypeIdx = i;
				break;
			}
		}

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = memTypeIdx;

		vkAllocateMemory(device, &allocInfo, nullptr, &memory);
		vkBindBufferMemory(device, buffer, memory, 0);
	}

	void VulkanVertexBuffer::create(const void *data, size_t size, BufferUsage usage)
	{
		this->m_size = size;
		this->m_hostVisible = (usage == BufferUsage::Dynamic || usage == BufferUsage::Stream);

		VkMemoryPropertyFlags memProps = this->m_hostVisible
											 ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
											 : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
					 memProps, this->m_buffer, this->m_memory);

		if (this->m_hostVisible)
		{
			vkMapMemory(VulkanState::getDevice(), this->m_memory, 0, size, 0, &this->m_mapped);
			if (data)
				memcpy(this->m_mapped, data, size);
		}
		else if (data)
		{
			// Stage upload: create temporary host-visible buffer, copy, then delete
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;
			createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
						 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
						 stagingBuffer, stagingMemory);

			void *mapped;
			vkMapMemory(VulkanState::getDevice(), stagingMemory, 0, size, 0, &mapped);
			memcpy(mapped, data, size);
			vkUnmapMemory(VulkanState::getDevice(), stagingMemory);

			// Copy via command buffer
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = VulkanState::getCommandPool();
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer cmd;
			vkAllocateCommandBuffers(VulkanState::getDevice(), &allocInfo, &cmd);

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			vkBeginCommandBuffer(cmd, &beginInfo);

			VkBufferCopy copyRegion{};
			copyRegion.size = size;
			vkCmdCopyBuffer(cmd, stagingBuffer, this->m_buffer, 1, &copyRegion);

			vkEndCommandBuffer(cmd);

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cmd;
			vkQueueSubmit(VulkanState::getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
			vkQueueWaitIdle(VulkanState::getGraphicsQueue());

			vkFreeCommandBuffers(VulkanState::getDevice(), VulkanState::getCommandPool(), 1, &cmd);
			vkDestroyBuffer(VulkanState::getDevice(), stagingBuffer, nullptr);
			vkFreeMemory(VulkanState::getDevice(), stagingMemory, nullptr);
		}
	}

	void VulkanVertexBuffer::update(const void *data, size_t size, size_t offset)
	{
		if (this->m_mapped && data)
			memcpy((char *)this->m_mapped + offset, data, size);
	}

	void VulkanVertexBuffer::bind()
	{
		VkCommandBuffer cmd = VulkanState::getCurrentCommandBuffer();
		if (cmd == VK_NULL_HANDLE)
			return;
		VkBuffer buffers[] = {this->m_buffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
	}

	void VulkanVertexBuffer::unbind() {}

	// --- Index Buffer ---

	VulkanIndexBuffer::~VulkanIndexBuffer()
	{
		VkDevice device = VulkanState::getDevice();
		if (device == VK_NULL_HANDLE)
			return;
		if (this->m_mapped)
			vkUnmapMemory(device, this->m_memory);
		if (this->m_buffer)
			vkDestroyBuffer(device, this->m_buffer, nullptr);
		if (this->m_memory)
			vkFreeMemory(device, this->m_memory, nullptr);
	}

	void VulkanIndexBuffer::create(const uint32_t *data, size_t count, BufferUsage)
	{
		this->m_count = count;
		VkDevice device = VulkanState::getDevice();
		VkDeviceSize size = count * sizeof(uint32_t);

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateBuffer(device, &bufferInfo, nullptr, &this->m_buffer);

		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device, this->m_buffer, &memReqs);

		VkPhysicalDeviceMemoryProperties memProps;
		vkGetPhysicalDeviceMemoryProperties(VulkanState::getPhysicalDevice(), &memProps);

		uint32_t memTypeIdx = 0;
		for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
		{
			if ((memReqs.memoryTypeBits & (1 << i)) &&
				(memProps.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
			{
				memTypeIdx = i;
				break;
			}
		}

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = memTypeIdx;

		vkAllocateMemory(device, &allocInfo, nullptr, &this->m_memory);
		vkBindBufferMemory(device, this->m_buffer, this->m_memory, 0);

		if (data)
		{
			vkMapMemory(device, this->m_memory, 0, size, 0, &this->m_mapped);
			memcpy(this->m_mapped, data, size);
		}
	}

	void VulkanIndexBuffer::bind()
	{
		VkCommandBuffer cmd = VulkanState::getCurrentCommandBuffer();
		if (cmd == VK_NULL_HANDLE)
			return;
		vkCmdBindIndexBuffer(cmd, this->m_buffer, 0, VK_INDEX_TYPE_UINT32);
	}

	void VulkanIndexBuffer::unbind() {}

	// --- Vertex Array ---

	void VulkanVertexArray::create() {}
	void VulkanVertexArray::bind()
	{
		if (this->m_vbo)
			this->m_vbo->bind();
		if (this->m_ibo)
			this->m_ibo->bind();
	}
	void VulkanVertexArray::unbind() {}

	void VulkanVertexArray::addVertexBuffer(std::shared_ptr<IVertexBuffer> vbo) { this->m_vbo = vbo; }
	void VulkanVertexArray::setIndexBuffer(std::shared_ptr<IIndexBuffer> ibo) { this->m_ibo = ibo; }
	void VulkanVertexArray::setupVertexLayout() {}

	void VulkanVertexArray::draw(PrimitiveType, size_t count)
	{
		VkCommandBuffer cmd = VulkanState::getCurrentCommandBuffer();
		if (cmd == VK_NULL_HANDLE)
			return;
		bind();
		vkCmdDraw(cmd, (uint32_t)count, 1, 0, 0);
	}

	void VulkanVertexArray::drawIndexed(PrimitiveType)
	{
		VkCommandBuffer cmd = VulkanState::getCurrentCommandBuffer();
		if (cmd == VK_NULL_HANDLE || !this->m_ibo)
			return;
		bind();
		vkCmdDrawIndexed(cmd, (uint32_t)this->m_ibo->getCount(), 1, 0, 0, 0);
	}

} // namespace GE::Graphics::Vulkan
