#pragma once
// GeniusEngine Core Types
// Aliases and common includes used across the engine

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <cstdint>
#include <iostream>

namespace GE
{

	// Math type aliases
	using Vec2 = glm::vec2;
	using Vec3 = glm::vec3;
	using Vec4 = glm::vec4;
	using Mat3 = glm::mat3;
	using Mat4 = glm::mat4;
	using Quat = glm::quat;

	using Color = glm::vec4; // RGBA

	// Vertex structure
	struct Vertex
	{
		Vec3 position;
		Vec3 normal;
		Vec2 texCoord;
		Color color{1.0f};
	};

	// Ray (for picking, raycasting)
	struct Ray
	{
		Vec3 origin;
		Vec3 direction;
	};

	// Transform
	struct Transform
	{
		Vec3 position{0.0f};
		Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
		Vec3 scale{1.0f};

		Mat4 matrix() const
		{
			Mat4 m = glm::translate(Mat4(1.0f), position);
			m *= glm::mat4_cast(rotation);
			m = glm::scale(m, scale);
			return m;
		}
	};

} // namespace GE
