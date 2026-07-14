#include "Mesh.hpp"
#include "../graphics/Factory.hpp"
#include <cmath>

namespace GE
{

	void Mesh::create(const std::vector<Vertex> &vertices,
					  const std::vector<uint32_t> &indices,
					  Graphics::BufferUsage usage)
	{
		this->m_vertices = vertices;
		this->m_vertexCount = vertices.size();
		this->m_indexCount = indices.size();

		this->m_vbo = Graphics::Factory::createVertexBuffer();
		this->m_vbo->create(vertices.data(), vertices.size() * sizeof(Vertex), usage);

		if (!indices.empty())
		{
			this->m_ibo = Graphics::Factory::createIndexBuffer();
			this->m_ibo->create(indices.data(), indices.size(), usage);
		}

		this->m_vao = Graphics::Factory::createVertexArray();
		this->m_vao->create();
		this->m_vao->addVertexBuffer(this->m_vbo);
		if (this->m_ibo)
			this->m_vao->setIndexBuffer(this->m_ibo);
		this->m_vao->setupVertexLayout();
	}

	void Mesh::createDynamic(size_t maxVertices, size_t maxIndices)
	{
		this->m_vertexCount = 0;
		this->m_indexCount = 0;

		this->m_vbo = Graphics::Factory::createVertexBuffer();
		this->m_vbo->create(nullptr, maxVertices * sizeof(Vertex), Graphics::BufferUsage::Dynamic);

		if (maxIndices > 0)
		{
			this->m_ibo = Graphics::Factory::createIndexBuffer();
			this->m_ibo->create(nullptr, maxIndices, Graphics::BufferUsage::Dynamic);
		}

		this->m_vao = Graphics::Factory::createVertexArray();
		this->m_vao->create();
		this->m_vao->addVertexBuffer(this->m_vbo);
		if (this->m_ibo)
			this->m_vao->setIndexBuffer(this->m_ibo);
		this->m_vao->setupVertexLayout();
	}

	void Mesh::updateVertices(const std::vector<Vertex> &vertices)
	{
		this->m_vertices = vertices;
		this->m_vertexCount = vertices.size();
		this->m_vbo->update(vertices.data(), vertices.size() * sizeof(Vertex));
	}

	Mesh Mesh::generateSphere(float radius, int sectors, int stacks, const Color &color, Graphics::BufferUsage usage)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		for (int i = 0; i <= stacks; ++i)
		{
			float stackAngle = glm::pi<float>() / 2.0f - (float)i * glm::pi<float>() / (float)stacks;
			float xy = radius * cosf(stackAngle);
			float z = radius * sinf(stackAngle);

			for (int j = 0; j <= sectors; ++j)
			{
				float sectorAngle = 2.0f * glm::pi<float>() * (float)j / (float)sectors;

				Vertex v;
				v.position.x = xy * cosf(sectorAngle);
				v.position.y = z;
				v.position.z = xy * sinf(sectorAngle);
				v.normal = glm::normalize(v.position);
				v.texCoord.x = (float)j / (float)sectors;
				v.texCoord.y = (float)i / (float)stacks;
				v.color = color;
				vertices.push_back(v);
			}
		}

		for (int i = 0; i < stacks; ++i)
		{
			int k1 = i * (sectors + 1);
			int k2 = k1 + sectors + 1;
			for (int j = 0; j < sectors; ++j, ++k1, ++k2)
			{
				if (i != 0)
				{
					indices.push_back(k1);
					indices.push_back(k2);
					indices.push_back(k1 + 1);
				}
				if (i != (stacks - 1))
				{
					indices.push_back(k1 + 1);
					indices.push_back(k2);
					indices.push_back(k2 + 1);
				}
			}
		}

		Mesh mesh;
		mesh.create(vertices, indices, usage);
		return mesh;
	}

	Mesh Mesh::generateGrid(float size, int divisions, const Color &color)
	{
		std::vector<Vertex> vertices;
		float step = size / (float)divisions;
		float half = size / 2.0f;

		for (int i = 0; i <= divisions; ++i)
		{
			float pos = -half + i * step;

			Vertex v1, v2, v3, v4;
			v1.position = {pos, 0.0f, -half};
			v1.normal = {0, 1, 0};
			v1.color = color;
			v2.position = {pos, 0.0f, half};
			v2.normal = {0, 1, 0};
			v2.color = color;
			v3.position = {-half, 0.0f, pos};
			v3.normal = {0, 1, 0};
			v3.color = color;
			v4.position = {half, 0.0f, pos};
			v4.normal = {0, 1, 0};
			v4.color = color;

			vertices.push_back(v1);
			vertices.push_back(v2);
			vertices.push_back(v3);
			vertices.push_back(v4);
		}

		Mesh mesh;
		mesh.create(vertices, {});
		return mesh;
	}

	Mesh Mesh::generateCircle(float radius, int segments, const Color &color)
	{
		std::vector<Vertex> vertices;
		for (int i = 0; i <= segments; ++i)
		{
			float angle = 2.0f * glm::pi<float>() * (float)i / (float)segments;
			Vertex v;
			v.position = {radius * cos(angle), 0.0f, radius * sin(angle)};
			v.normal = {0, 1, 0};
			v.color = color;
			vertices.push_back(v);
		}

		Mesh mesh;
		mesh.create(vertices, {});
		return mesh;
	}

	void Mesh::draw(Graphics::PrimitiveType primitive)
	{
		if (this->m_indexCount > 0)
		{
			this->m_vao->drawIndexed(primitive);
		}
		else
		{
			this->m_vao->draw(primitive, this->m_vertexCount);
		}
	}

	void Mesh::drawLines()
	{
		this->m_vao->draw(Graphics::PrimitiveType::LineStrip, this->m_vertexCount);
	}

	Mesh Mesh::generateRing(float innerRadius, float outerRadius, int segments, const Color &innerColor, const Color &outerColor, Graphics::BufferUsage usage)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		for (int i = 0; i <= segments; ++i)
		{
			float angle = 2.0f * glm::pi<float>() * (float)i / (float)segments;
			float c = cosf(angle);
			float s = sinf(angle);

			// Inner vertex
			Vertex vi;
			vi.position = {innerRadius * c, 0.0f, innerRadius * s};
			vi.normal = {0, 1, 0};
			vi.texCoord = {0.0f, (float)i / (float)segments};
			vi.color = innerColor;
			vertices.push_back(vi);

			// Outer vertex
			Vertex vo;
			vo.position = {outerRadius * c, 0.0f, outerRadius * s};
			vo.normal = {0, 1, 0};
			vo.texCoord = {1.0f, (float)i / (float)segments};
			vo.color = outerColor;
			vertices.push_back(vo);
		}

		for (int i = 0; i < segments; ++i)
		{
			uint32_t i0 = i * 2;
			uint32_t i1 = i * 2 + 1;
			uint32_t i2 = (i + 1) * 2;
			uint32_t i3 = (i + 1) * 2 + 1;

			// Two triangles per segment, both sides
			indices.push_back(i0);
			indices.push_back(i2);
			indices.push_back(i1);
			indices.push_back(i1);
			indices.push_back(i2);
			indices.push_back(i3);

			// Back face
			indices.push_back(i0);
			indices.push_back(i1);
			indices.push_back(i2);
			indices.push_back(i1);
			indices.push_back(i3);
			indices.push_back(i2);
		}

		Mesh mesh;
		mesh.create(vertices, indices, usage);
		return mesh;
	}

} // namespace GE
