#pragma once

#include "../../engine/Mesh.hpp"
#include <deque>

struct TrailRenderer
{
	static constexpr size_t MAX_TRAIL_POINTS = 500;
	Mesh mesh;
	std::vector<Vertex> vertices;

	void init()
	{
		mesh.createDynamic(MAX_TRAIL_POINTS);
		vertices.reserve(MAX_TRAIL_POINTS);
	}

	void update(const std::deque<Vec3> &trail, const Color &color)
	{
		vertices.clear();
		for (size_t i = 0; i < trail.size(); ++i)
		{
			float alpha = (float)i / (float)trail.size();
			Vertex v;
			v.position = trail[i];
			v.normal = {0, 1, 0};
			v.color = color;
			v.color.a = alpha * 0.8f;
			vertices.push_back(v);
		}
		if (!vertices.empty())
			mesh.updateVertices(vertices);
	}
};
