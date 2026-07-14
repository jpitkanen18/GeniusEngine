#pragma once
// GeniusEngine - Renderer (high-level draw calls, backend-agnostic)

#include "../core/Types.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

namespace GE
{

	struct Light
	{
		Vec3 position{0.0f, 10.0f, 0.0f};
		Color color{1.0f, 1.0f, 1.0f, 1.0f};
		float intensity = 1.0f;
	};

	class Renderer
	{
	public:
		Renderer() = default;

		void init();
		void beginScene(Camera &camera);
		void endScene();

		void setLight(const Light &light) { this->m_light = light; }

		// High-level draw: material handles shader binding
		void drawMesh(Mesh &mesh, Material &material, const Transform &transform);

		// Simple draw with a provided shader (for trails, grids, etc.)
		void drawMeshSimple(Mesh &mesh, Graphics::IShader *shader, const Transform &transform);

		// Built-in shaders
		std::shared_ptr<Graphics::IShader> createDefaultShader();
		std::shared_ptr<Graphics::IShader> createUnlitShader();
		std::shared_ptr<Graphics::IShader> createLineShader();

	private:
		Camera *m_camera = nullptr;
		Light m_light;
		Mat4 m_viewProj{1.0f};
	};

} // namespace GE
