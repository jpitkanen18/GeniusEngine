#include "Renderer.hpp"
#include "AssetLoader.hpp"
#include "../graphics/Factory.hpp"
#include <glad/gl.h>

namespace GE
{

	void Renderer::init()
	{
		if (Graphics::Factory::getBackend() == Graphics::Backend::OpenGL)
		{
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_LINE_SMOOTH);
			glLineWidth(1.5f);
		}
	}

	void Renderer::beginScene(Camera &camera)
	{
		this->m_camera = &camera;
		this->m_viewProj = camera.getViewProjection();
	}

	void Renderer::endScene()
	{
		this->m_camera = nullptr;
	}

	void Renderer::drawMesh(Mesh &mesh, Material &material, const Transform &transform)
	{
		material.bind();
		auto *shader = material.getShader();
		if (shader)
		{
			shader->setMat4("u_viewProjection", this->m_viewProj);
			shader->setMat4("u_model", transform.matrix());
			shader->setMat3("u_normalMatrix", Mat3(glm::transpose(glm::inverse(transform.matrix()))));
			shader->setVec3("u_lightPos", this->m_light.position);
			shader->setVec4("u_lightColor", this->m_light.color * this->m_light.intensity);
			if (this->m_camera)
				shader->setVec3("u_viewPos", this->m_camera->getPosition());
		}
		mesh.draw();
		material.unbind();
	}

	void Renderer::drawMeshSimple(Mesh &mesh, Graphics::IShader *shader, const Transform &transform)
	{
		if (shader)
		{
			shader->bindForLines();
			shader->setMat4("u_viewProjection", this->m_viewProj);
			shader->setMat4("u_model", transform.matrix());
		}
		mesh.drawLines();
		if (shader)
			shader->unbind();
	}

	std::shared_ptr<Graphics::IShader> Renderer::createDefaultShader()
	{
		return ShaderLoader::load("default");
	}

	std::shared_ptr<Graphics::IShader> Renderer::createUnlitShader()
	{
		return ShaderLoader::load("unlit");
	}

	std::shared_ptr<Graphics::IShader> Renderer::createLineShader()
	{
		return ShaderLoader::load("line");
	}

} // namespace GE
