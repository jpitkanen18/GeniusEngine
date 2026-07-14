#pragma once
// GeniusEngine - Camera (backend-agnostic)

#include "../core/Types.hpp"

namespace GE
{

	enum class ProjectionType
	{
		Perspective,
		Orthographic
	};

	class Camera
	{
	public:
		Camera() = default;
		virtual ~Camera() = default;

		void setPerspective(float fov, float aspect, float nearPlane, float farPlane);
		void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);

		void setPosition(const Vec3 &pos)
		{
			this->m_position = pos;
			this->m_dirty = true;
		}
		void setTarget(const Vec3 &target)
		{
			this->m_target = target;
			this->m_dirty = true;
		}
		void setUp(const Vec3 &up)
		{
			this->m_up = up;
			this->m_dirty = true;
		}

		void lookAt(const Vec3 &pos, const Vec3 &target, const Vec3 &up);

		// Orbit around target
		void orbit(float deltaYaw, float deltaPitch, float deltaDistance);

		// Free-fly movement (WASD-style)
		void moveForward(float amount);
		void moveRight(float amount);
		void moveUp(float amount);
		void rotateYaw(float angle);
		void rotatePitch(float angle);

		// FPS-style mouse look (rotates camera direction)
		void mouseLook(float deltaX, float deltaY);

		// Follow a world position (camera orbits around it)
		void follow(const Vec3 &targetPos);

		// Screen-to-world ray (for picking/clicking on objects)
		Ray screenToRay(float screenX, float screenY, float windowW, float windowH);

		virtual const Mat4 &getViewMatrix();
		const Mat4 &getProjectionMatrix() const { return this->m_projection; }
		Mat4 getViewProjection();

		Vec3 getPosition() const { return this->m_position; }
		Vec3 getTarget() const { return this->m_target; }
		Vec3 getForward() const { return glm::normalize(this->m_target - this->m_position); }
		Vec3 getRight() const { return glm::normalize(glm::cross(getForward(), this->m_up)); }
		Vec3 getUp() const { return this->m_up; }

		float getDistance() const { return glm::length(this->m_target - this->m_position); }

	protected:
		Vec3 m_position{0.0f, 5.0f, 20.0f};
		Vec3 m_target{0.0f, 0.0f, 0.0f};
		Vec3 m_up{0.0f, 1.0f, 0.0f};

		Mat4 m_view{1.0f};
		Mat4 m_projection{1.0f};
		ProjectionType m_projType = ProjectionType::Perspective;
		float m_fov = 60.0f;
		float m_aspect = 16.0f / 9.0f;
		float m_near = 0.1f;
		float m_far = 10000.0f;
		bool m_dirty = true;
	};

} // namespace GE
