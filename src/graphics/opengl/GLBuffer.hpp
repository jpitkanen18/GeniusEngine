#pragma once
// OpenGL Buffer Implementation

#include "../Buffer.hpp"
#include <glad/gl.h>

namespace GE::Graphics::GL
{

	class GLVertexBuffer : public IVertexBuffer
	{
	public:
		GLVertexBuffer() = default;
		~GLVertexBuffer() override;

		void create(const void *data, size_t size, BufferUsage usage) override;
		void update(const void *data, size_t size, size_t offset = 0) override;
		void bind() override;
		void unbind() override;

		static GLenum toGL(BufferUsage usage);

	private:
		GLuint m_vbo = 0;
	};

	class GLIndexBuffer : public IIndexBuffer
	{
	public:
		GLIndexBuffer() = default;
		~GLIndexBuffer() override;

		void create(const uint32_t *data, size_t count, BufferUsage usage) override;
		void bind() override;
		void unbind() override;
		size_t getCount() const override { return this->m_count; }

	private:
		GLuint m_ebo = 0;
		size_t m_count = 0;
	};

	class GLVertexArray : public IVertexArray
	{
	public:
		GLVertexArray() = default;
		~GLVertexArray() override;

		void create() override;
		void bind() override;
		void unbind() override;
		void addVertexBuffer(std::shared_ptr<IVertexBuffer> vbo) override;
		void setIndexBuffer(std::shared_ptr<IIndexBuffer> ibo) override;
		void setupVertexLayout() override;
		void draw(PrimitiveType primitive, size_t count) override;
		void drawIndexed(PrimitiveType primitive) override;

	private:
		GLuint m_vao = 0;
		std::shared_ptr<IVertexBuffer> m_vbo;
		std::shared_ptr<IIndexBuffer> m_ibo;
		static GLenum toGL(PrimitiveType type);
	};

} // namespace GE::Graphics::GL
