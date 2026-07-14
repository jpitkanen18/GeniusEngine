#include "Camera.hpp"

namespace GE
{

	void Camera::setPerspective(float fov, float aspect, float nearPlane, float farPlane)
	{
		this->m_projType = ProjectionType::Perspective;
		this->m_fov = fov;
		this->m_aspect = aspect;
		this->m_near = nearPlane;
		this->m_far = farPlane;
		this->m_projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
	}

	void Camera::setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
	{
		this->m_projType = ProjectionType::Orthographic;
		this->m_projection = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
	}

	void Camera::lookAt(const Vec3 &pos, const Vec3 &target, const Vec3 &up)
	{
		this->m_position = pos;
		this->m_target = target;
		this->m_up = up;
		this->m_dirty = true;
	}

	void Camera::orbit(float deltaYaw, float deltaPitch, float deltaDistance)
	{
		Vec3 offset = this->m_position - this->m_target;
		float distance = glm::length(offset);

		// Spherical coordinates
		float theta = atan2(offset.x, offset.z);
		float phi = acos(glm::clamp(offset.y / distance, -1.0f, 1.0f));

		theta += deltaYaw;
		phi += deltaPitch;
		phi = glm::clamp(phi, 0.1f, 3.04f); // Avoid gimbal lock
		distance += deltaDistance;
		distance = glm::max(distance, 1.0f);

		this->m_position.x = this->m_target.x + distance * sin(phi) * sin(theta);
		this->m_position.y = this->m_target.y + distance * cos(phi);
		this->m_position.z = this->m_target.z + distance * sin(phi) * cos(theta);
		this->m_dirty = true;
	}

	void Camera::moveForward(float amount)
	{
		Vec3 fwd = getForward();
		this->m_position += fwd * amount;
		this->m_target += fwd * amount;
		this->m_dirty = true;
	}

	void Camera::moveRight(float amount)
	{
		Vec3 right = getRight();
		this->m_position += right * amount;
		this->m_target += right * amount;
		this->m_dirty = true;
	}

	void Camera::moveUp(float amount)
	{
		this->m_position += this->m_up * amount;
		this->m_target += this->m_up * amount;
		this->m_dirty = true;
	}

	void Camera::rotateYaw(float angle)
	{
		Vec3 offset = this->m_target - this->m_position;
		float c = cos(angle), s = sin(angle);
		Vec3 rotated;
		rotated.x = offset.x * c - offset.z * s;
		rotated.y = offset.y;
		rotated.z = offset.x * s + offset.z * c;
		this->m_target = this->m_position + rotated;
		this->m_dirty = true;
	}

	void Camera::rotatePitch(float angle)
	{
		Vec3 fwd = getForward();
		Vec3 right = getRight();
		float c = cos(angle), s = sin(angle);
		Vec3 newFwd = fwd * c + this->m_up * s;
		// Prevent flipping
		if (std::abs(glm::dot(glm::normalize(newFwd), Vec3(0, 1, 0))) < 0.99f)
		{
			float dist = glm::length(this->m_target - this->m_position);
			this->m_target = this->m_position + glm::normalize(newFwd) * dist;
			this->m_dirty = true;
		}
	}

	void Camera::mouseLook(float deltaX, float deltaY)
	{
		rotateYaw(deltaX);
		rotatePitch(-deltaY);
	}

	void Camera::follow(const Vec3 &targetPos)
	{
		Vec3 offset = this->m_position - this->m_target;
		this->m_target = targetPos;
		this->m_position = targetPos + offset;
		this->m_dirty = true;
	}

	Ray Camera::screenToRay(float screenX, float screenY, float windowW, float windowH)
	{
		// Normalized device coordinates
		float ndcX = (2.0f * screenX) / windowW - 1.0f;
		float ndcY = 1.0f - (2.0f * screenY) / windowH;

		// Unproject near and far points
		Mat4 invVP = glm::inverse(getViewProjection());
		Vec4 nearPoint = invVP * Vec4(ndcX, ndcY, -1.0f, 1.0f);
		Vec4 farPoint = invVP * Vec4(ndcX, ndcY, 1.0f, 1.0f);
		nearPoint /= nearPoint.w;
		farPoint /= farPoint.w;

		Ray ray;
		ray.origin = Vec3(nearPoint);
		ray.direction = glm::normalize(Vec3(farPoint) - Vec3(nearPoint));
		return ray;
	}

	const Mat4 &Camera::getViewMatrix()
	{
		if (this->m_dirty)
		{
			this->m_view = glm::lookAt(this->m_position, this->m_target, this->m_up);
			this->m_dirty = false;
		}
		return this->m_view;
	}

	Mat4 Camera::getViewProjection()
	{
		return this->m_projection * getViewMatrix();
	}

} // namespace GE
