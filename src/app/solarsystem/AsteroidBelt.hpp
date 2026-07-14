#pragma once

#include "../../engine/Mesh.hpp"
#include "../../engine/Material.hpp"
#include "../../engine/Renderer.hpp"
#include <vector>
#include <random>
#include <omp.h>

using namespace GE;

struct AsteroidBelt
{
	static constexpr int COUNT = 1000;
	static constexpr int VERTS_PER_ASTEROID = 12;
	static constexpr float INNER_RADIUS = 18.0f;
	static constexpr float OUTER_RADIUS = 25.0f;

	struct Asteroid
	{
		float orbitRadius;
		float angle;
		float speed;
		float inclination;
		float size;
		float tumblePhase;
		Vec3 position;
	};

	std::vector<Asteroid> asteroids;
	std::vector<Vertex> vertices;
	Mesh mesh;
	Material material;
	bool visible = true;

	void init(std::shared_ptr<Graphics::IShader> shader)
	{
		asteroids.resize(COUNT);
		vertices.resize(COUNT * VERTS_PER_ASTEROID);

		std::mt19937 rng(42);
		std::uniform_real_distribution<float> radiusDist(INNER_RADIUS, OUTER_RADIUS);
		std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
		std::uniform_real_distribution<float> incDist(-0.08f, 0.08f);
		std::uniform_real_distribution<float> sizeDist(0.05f, 0.25f);
		std::uniform_real_distribution<float> speedVar(0.7f, 1.3f);

		for (int i = 0; i < COUNT; ++i)
		{
			auto &a = asteroids[i];
			a.orbitRadius = radiusDist(rng);
			a.angle = angleDist(rng);
			a.inclination = incDist(rng);
			a.size = sizeDist(rng);
			a.tumblePhase = angleDist(rng);
			float baseSpeed = 2.0f * glm::pi<float>() / (40.0f * std::sqrt(a.orbitRadius / INNER_RADIUS));
			a.speed = baseSpeed * speedVar(rng);
			a.position = Vec3(0.0f);
		}

		mesh.createDynamic(COUNT * VERTS_PER_ASTEROID);

		material.setShader(shader);
		MaterialProperties props;
		props.diffuse = {0.55f, 0.5f, 0.45f, 1.0f};
		props.ambient = {0.2f, 0.18f, 0.15f, 1.0f};
		props.specular = {0.3f, 0.3f, 0.3f, 1.0f};
		props.shininess = 8.0f;
		material.setProperties(props);
	}

	void update(float dt, float timeScale)
	{
#pragma omp parallel for schedule(static)
		for (int i = 0; i < COUNT; ++i)
		{
			auto &a = asteroids[i];
			a.angle += a.speed * dt * timeScale;
			a.position.x = a.orbitRadius * std::cos(a.angle);
			a.position.z = a.orbitRadius * std::sin(a.angle);
			a.position.y = a.orbitRadius * std::sin(a.angle) * a.inclination;
		}

#pragma omp parallel for schedule(static)
		for (int i = 0; i < COUNT; ++i)
		{
			const auto &a = asteroids[i];
			int base = i * VERTS_PER_ASTEROID;

			float phase = a.angle * 3.0f + a.tumblePhase;
			float cp = std::cos(phase), sp = std::sin(phase);
			float s = a.size;
			Vec3 top = a.position + Vec3(0, s * 1.2f, 0);
			Vec3 v0 = a.position + Vec3(s * cp, -s * 0.4f, s * sp);
			Vec3 v1 = a.position + Vec3(-s * sp, -s * 0.3f, s * cp);
			Vec3 v2 = a.position + Vec3(-s * cp * 0.7f, -s * 0.5f, -s * sp * 0.8f);

			float shade = 0.4f + 0.2f * (float)(i % 7) / 6.0f;
			Color baseCol = {shade, shade * 0.9f, shade * 0.8f, 1.0f};

			// Sun is at origin — compute toSun direction for this asteroid
			Vec3 toSun = glm::normalize(-a.position);

			auto makeVert = [&](Vec3 pos, Vec3 faceNormal) -> Vertex
			{
				Vertex vert;
				vert.position = pos;
				Vec3 n = glm::normalize(faceNormal);
				vert.normal = n;
				vert.texCoord = {0, 0};
				// Bake sun diffuse into vertex color
				float NdotL = glm::dot(n, toSun);
				float lit = 0.05f + std::max(0.0f, NdotL) * 0.95f;
				vert.color = Color(baseCol.r * lit, baseCol.g * lit, baseCol.b * lit, 1.0f);
				return vert;
			};

			Vec3 n1 = glm::cross(v0 - top, v1 - top);
			vertices[base + 0] = makeVert(top, n1);
			vertices[base + 1] = makeVert(v0, n1);
			vertices[base + 2] = makeVert(v1, n1);

			Vec3 n2 = glm::cross(v1 - top, v2 - top);
			vertices[base + 3] = makeVert(top, n2);
			vertices[base + 4] = makeVert(v1, n2);
			vertices[base + 5] = makeVert(v2, n2);

			Vec3 n3 = glm::cross(v2 - top, v0 - top);
			vertices[base + 6] = makeVert(top, n3);
			vertices[base + 7] = makeVert(v2, n3);
			vertices[base + 8] = makeVert(v0, n3);

			Vec3 n4 = glm::cross(v2 - v0, v1 - v0);
			vertices[base + 9] = makeVert(v0, n4);
			vertices[base + 10] = makeVert(v2, n4);
			vertices[base + 11] = makeVert(v1, n4);
		}

		mesh.updateVertices(vertices);
	}

	void draw(Renderer &renderer)
	{
		if (!visible)
			return;
		Transform t;
		renderer.drawMesh(mesh, material, t);
	}
};
