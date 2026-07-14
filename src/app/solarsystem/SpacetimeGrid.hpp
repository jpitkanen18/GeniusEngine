#pragma once

#include "../../engine/Mesh.hpp"
#include "CelestialBody.hpp"
#include <vector>
#include <omp.h>

struct SpacetimeGrid
{
	static constexpr int GRID_SIZE = 120;
	static constexpr float GRID_EXTENT = 80.0f;
	static constexpr float GRID_Y_OFFSET = -5.0f;

	Mesh mesh;
	std::vector<Vertex> vertices;
	bool visible = true;
	float curvatureStrength = 2.0f;

	void init()
	{
		float step = (2.0f * GRID_EXTENT) / (float)(GRID_SIZE - 1);
		vertices.resize(GRID_SIZE * GRID_SIZE);

		for (int z = 0; z < GRID_SIZE; ++z)
		{
			for (int x = 0; x < GRID_SIZE; ++x)
			{
				int idx = z * GRID_SIZE + x;
				float px = -GRID_EXTENT + x * step;
				float pz = -GRID_EXTENT + z * step;
				vertices[idx].position = {px, GRID_Y_OFFSET, pz};
				vertices[idx].normal = {0, 1, 0};
				vertices[idx].color = {0.1f, 0.3f, 0.6f, 0.4f};
			}
		}

		mesh.createDynamic(GRID_SIZE * GRID_SIZE);
		mesh.updateVertices(vertices);
	}

	void update(const std::vector<CelestialBody> &bodies)
	{
		float step = (2.0f * GRID_EXTENT) / (float)(GRID_SIZE - 1);

#pragma omp parallel for schedule(static) collapse(2)
		for (int z = 0; z < GRID_SIZE; ++z)
		{
			for (int x = 0; x < GRID_SIZE; ++x)
			{
				int idx = z * GRID_SIZE + x;
				float px = -GRID_EXTENT + x * step;
				float pz = -GRID_EXTENT + z * step;

				float depression = 0.0f;
				for (const auto &body : bodies)
				{
					float dx = px - body.position.x;
					float dz = pz - body.position.z;
					float r2 = dx * dx + dz * dz;
					float r = std::sqrt(r2);
					float rs = body.radius * 1.5f;
					float wellDepth = curvatureStrength * body.mass / std::pow(r + rs, 1.4f);

					if (body.mass > 50.0f)
					{
						float falloff = std::exp(-r * 0.04f);
						wellDepth *= (0.3f + 0.7f * falloff);
					}
					depression -= wellDepth;
				}

				depression = std::max(depression, -30.0f);
				vertices[idx].position = {px, GRID_Y_OFFSET + depression, pz};

				float t = std::min(1.0f, -depression / 15.0f);
				float deep = std::min(1.0f, -depression / 25.0f);
				vertices[idx].color = {
					0.05f + deep * 0.7f,
					0.15f + (1.0f - t) * 0.4f + t * 0.1f,
					0.5f + t * 0.4f,
					0.25f + t * 0.5f};
			}
		}

		mesh.updateVertices(vertices);
	}
};
