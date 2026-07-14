#pragma once
// GeniusEngine - Mesh abstraction (backend-agnostic)
// Wraps vertex/index data and provides generation utilities

#include "../core/Types.hpp"
#include "../graphics/Buffer.hpp"

namespace GE
{

	class Mesh
	{
	public:
		Mesh() = default;
		virtual ~Mesh() = default;

		// Build from raw data
		void create(const std::vector<Vertex> &vertices,
					const std::vector<uint32_t> &indices,
					Graphics::BufferUsage usage = Graphics::BufferUsage::Static);

		// Build dynamic (for trails, etc.)
		void createDynamic(size_t maxVertices, size_t maxIndices = 0);
		void updateVertices(const std::vector<Vertex> &vertices);

		// Access stored vertices (CPU-side copy for readback/manipulation)
		const std::vector<Vertex> &getVertices() const { return this->m_vertices; }
		std::vector<Vertex> &getVertices() { return this->m_vertices; }

		// Primitive generators
		static Mesh generateSphere(float radius, int sectors, int stacks, const Color &color = {1, 1, 1, 1},
								   Graphics::BufferUsage usage = Graphics::BufferUsage::Static);
		static Mesh generateGrid(float size, int divisions, const Color &color = {0.3f, 0.3f, 0.3f, 1.0f});
		static Mesh generateCircle(float radius, int segments, const Color &color = {1, 1, 1, 1});
		static Mesh generateRing(float innerRadius, float outerRadius, int segments, const Color &innerColor, const Color &outerColor,
								 Graphics::BufferUsage usage = Graphics::BufferUsage::Static);

		virtual void draw(Graphics::PrimitiveType primitive = Graphics::PrimitiveType::Triangles);
		virtual void drawLines();

		size_t getVertexCount() const { return this->m_vertexCount; }
		bool hasIndices() const { return this->m_indexCount > 0; }

	protected:
		std::shared_ptr<Graphics::IVertexArray> m_vao;
		std::shared_ptr<Graphics::IVertexBuffer> m_vbo;
		std::shared_ptr<Graphics::IIndexBuffer> m_ibo;
		std::vector<Vertex> m_vertices; // CPU-side copy
		size_t m_vertexCount = 0;
		size_t m_indexCount = 0;
	};

} // namespace GE
