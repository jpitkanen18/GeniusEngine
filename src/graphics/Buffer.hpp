#pragma once
// GeniusEngine Buffer Interface

#include "../core/Types.hpp"

namespace GE::Graphics
{

	enum class BufferUsage
	{
		Static,
		Dynamic,
		Stream
	};

	enum class PrimitiveType
	{
		Triangles,
		Lines,
		LineStrip,
		Points
	};

	class IVertexBuffer
	{
	public:
		virtual ~IVertexBuffer() = default;
		virtual void create(const void *data, size_t size, BufferUsage usage) = 0;
		virtual void update(const void *data, size_t size, size_t offset = 0) = 0;
		virtual void bind() = 0;
		virtual void unbind() = 0;
	};

	class IIndexBuffer
	{
	public:
		virtual ~IIndexBuffer() = default;
		virtual void create(const uint32_t *data, size_t count, BufferUsage usage) = 0;
		virtual void bind() = 0;
		virtual void unbind() = 0;
		virtual size_t getCount() const = 0;
	};

	class IVertexArray
	{
	public:
		virtual ~IVertexArray() = default;
		virtual void create() = 0;
		virtual void bind() = 0;
		virtual void unbind() = 0;
		virtual void addVertexBuffer(std::shared_ptr<IVertexBuffer> vbo) = 0;
		virtual void setIndexBuffer(std::shared_ptr<IIndexBuffer> ibo) = 0;

		// Configure vertex attribute layout for Vertex struct
		virtual void setupVertexLayout() = 0;

		virtual void draw(PrimitiveType primitive, size_t count) = 0;
		virtual void drawIndexed(PrimitiveType primitive) = 0;
	};

} // namespace GE::Graphics
