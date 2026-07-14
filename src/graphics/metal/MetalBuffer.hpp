#pragma once
// Metal Buffer Implementation

#include "../Buffer.hpp"

#ifdef __APPLE__

namespace GE::Graphics::Metal
{

	class MetalVertexBuffer : public IVertexBuffer
	{
	public:
		MetalVertexBuffer() = default;
		~MetalVertexBuffer() override;

		void create(const void *data, size_t size, BufferUsage usage) override;
		void update(const void *data, size_t size, size_t offset = 0) override;
		void bind() override;
		void unbind() override;

		void setDevice(void *device) { this->m_device = device; }
		void setEncoder(void *encoder) { this->m_encoder = encoder; }
		void *getBuffer() { return this->m_buffer; }
		size_t getSize() const { return this->m_size; }

	private:
		void *m_device = nullptr;
		void *m_encoder = nullptr;
		void *m_buffer = nullptr;
		size_t m_size = 0;
	};

	class MetalIndexBuffer : public IIndexBuffer
	{
	public:
		MetalIndexBuffer() = default;
		~MetalIndexBuffer() override;

		void create(const uint32_t *data, size_t count, BufferUsage usage) override;
		void bind() override;
		void unbind() override;
		size_t getCount() const override { return this->m_count; }

		void setDevice(void *device) { this->m_device = device; }
		void setEncoder(void *encoder) { this->m_encoder = encoder; }
		void *getBuffer() { return this->m_buffer; }

	private:
		void *m_device = nullptr;
		void *m_encoder = nullptr;
		void *m_buffer = nullptr;
		size_t m_count = 0;
	};

	class MetalVertexArray : public IVertexArray
	{
	public:
		MetalVertexArray() = default;
		~MetalVertexArray() override = default;

		void create() override;
		void bind() override;
		void unbind() override;
		void addVertexBuffer(std::shared_ptr<IVertexBuffer> vbo) override;
		void setIndexBuffer(std::shared_ptr<IIndexBuffer> ibo) override;
		void setupVertexLayout() override;
		void draw(PrimitiveType primitive, size_t count) override;
		void drawIndexed(PrimitiveType primitive) override;

		void setEncoder(void *encoder) { this->m_encoder = encoder; }

	private:
		void *m_encoder = nullptr;
		std::shared_ptr<IVertexBuffer> m_vbo;
		std::shared_ptr<IIndexBuffer> m_ibo;

		static int toMTL(PrimitiveType type);
	};

} // namespace GE::Graphics::Metal

#endif // __APPLE__
