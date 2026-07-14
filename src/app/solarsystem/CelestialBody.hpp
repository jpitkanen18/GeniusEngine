#pragma once

#include "../../engine/Mesh.hpp"
#include "../../engine/Material.hpp"
#include <deque>
#include <string>

using namespace GE;

struct CelestialBody
{
	std::string name;
	float orbitRadius;
	float radius;
	float orbitalPeriod;
	float inclination;
	Color color;
	float mass;

	float angle = 0.0f;
	Vec3 position{0.0f};
	std::deque<Vec3> trail;
	Mesh sphereMesh;
	Material material;

	// Optional ring (e.g. Saturn)
	bool hasRing = false;
	Mesh ringMesh;
	Material ringMaterial;
	float ringTilt = 0.0f; // degrees
	float ringInnerRadius = 0.0f;
	float ringOuterRadius = 0.0f;
	std::vector<Vertex> ringBaseVertices; // original ring vertex colors for shadow computation
};
