#include "GLBuffer.hpp"

namespace GE::Graphics::GL
{

	// --- GLVertexBuffer ---

	GLVertexBuffer::~GLVertexBuffer()
	{
		if (this->m_vbo)
			glDeleteBuffers(1, &this->m_vbo);
	}

	GLenum GLVertexBuffer::toGL(BufferUsage usage)
	{
		switch (usage)
		{
		case BufferUsage::Static:
			return GL_STATIC_DRAW;
		case BufferUsage::Dynamic:
			return GL_DYNAMIC_DRAW;
		case BufferUsage::Stream:
			return GL_STREAM_DRAW;
		}
		return GL_STATIC_DRAW;
	}

	void GLVertexBuffer::create(const void *data, size_t size, BufferUsage usage)
	{
		glGenBuffers(1, &this->m_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, this->m_vbo);
		glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(size), data, toGL(usage));
	}

	void GLVertexBuffer::update(const void *data, size_t size, size_t offset)
	{
		glBindBuffer(GL_ARRAY_BUFFER, this->m_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), data);
	}

	void GLVertexBuffer::bind() { glBindBuffer(GL_ARRAY_BUFFER, this->m_vbo); }
	void GLVertexBuffer::unbind() { glBindBuffer(GL_ARRAY_BUFFER, 0); }

	// --- GLIndexBuffer ---

	GLIndexBuffer::~GLIndexBuffer()
	{
		if (this->m_ebo)
			glDeleteBuffers(1, &this->m_ebo);
	}

	void GLIndexBuffer::create(const uint32_t *data, size_t count, BufferUsage usage)
	{
		this->m_count = count;
		glGenBuffers(1, &this->m_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					 static_cast<GLsizeiptr>(count * sizeof(uint32_t)),
					 data, GLVertexBuffer::toGL(usage));
	}

	void GLIndexBuffer::bind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->m_ebo); }
	void GLIndexBuffer::unbind() { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

	// --- GLVertexArray ---

	GLVertexArray::~GLVertexArray()
	{
		if (this->m_vao)
			glDeleteVertexArrays(1, &this->m_vao);
	}

	void GLVertexArray::create()
	{
		glGenVertexArrays(1, &this->m_vao);
	}

	void GLVertexArray::bind() { glBindVertexArray(this->m_vao); }
	void GLVertexArray::unbind() { glBindVertexArray(0); }

	void GLVertexArray::addVertexBuffer(std::shared_ptr<IVertexBuffer> vbo)
	{
		this->m_vbo = vbo;
	}

	void GLVertexArray::setIndexBuffer(std::shared_ptr<IIndexBuffer> ibo)
	{
		this->m_ibo = ibo;
	}

	void GLVertexArray::setupVertexLayout()
	{
		bind();
		this->m_vbo->bind();

		// Position: location 0
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
							  (void *)offsetof(Vertex, position));

		// Normal: location 1
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
							  (void *)offsetof(Vertex, normal));

		// TexCoord: location 2
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
							  (void *)offsetof(Vertex, texCoord));

		// Color: location 3
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
							  (void *)offsetof(Vertex, color));

		if (this->m_ibo)
			this->m_ibo->bind();
		unbind();
	}

	GLenum GLVertexArray::toGL(PrimitiveType type)
	{
		switch (type)
		{
		case PrimitiveType::Triangles:
			return GL_TRIANGLES;
		case PrimitiveType::Lines:
			return GL_LINES;
		case PrimitiveType::LineStrip:
			return GL_LINE_STRIP;
		case PrimitiveType::Points:
			return GL_POINTS;
		}
		return GL_TRIANGLES;
	}

	void GLVertexArray::draw(PrimitiveType primitive, size_t count)
	{
		bind();
		glDrawArrays(toGL(primitive), 0, static_cast<GLsizei>(count));
		unbind();
	}

	void GLVertexArray::drawIndexed(PrimitiveType primitive)
	{
		bind();
		glDrawElements(toGL(primitive), static_cast<GLsizei>(this->m_ibo->getCount()), GL_UNSIGNED_INT, nullptr);
		unbind();
	}

} // namespace GE::Graphics::GL
