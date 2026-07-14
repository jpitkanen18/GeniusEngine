#include "Material.hpp"

namespace GE
{

	void Material::bind()
	{
		if (this->m_shader)
		{
			this->m_shader->bind();
			this->m_shader->setVec4("u_material.ambient", this->m_props.ambient);
			this->m_shader->setVec4("u_material.diffuse", this->m_props.diffuse);
			this->m_shader->setVec4("u_material.specular", this->m_props.specular);
			this->m_shader->setVec4("u_material.emissive", this->m_props.emissive);
			this->m_shader->setFloat("u_material.shininess", this->m_props.shininess);
		}
		if (this->m_texture)
		{
			this->m_texture->bind(0);
		}
	}

	void Material::unbind()
	{
		if (this->m_texture)
			this->m_texture->unbind();
		if (this->m_shader)
			this->m_shader->unbind();
	}

} // namespace GE
