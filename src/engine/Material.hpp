#pragma once
// GeniusEngine - Material (shader + properties, backend-agnostic)

#include "../core/Types.hpp"
#include "../graphics/Shader.hpp"
#include "../graphics/Texture.hpp"

namespace GE
{

	struct MaterialProperties
	{
		Color ambient{0.1f, 0.1f, 0.1f, 1.0f};
		Color diffuse{0.8f, 0.8f, 0.8f, 1.0f};
		Color specular{1.0f, 1.0f, 1.0f, 1.0f};
		Color emissive{0.0f, 0.0f, 0.0f, 1.0f};
		float shininess = 32.0f;
	};

	class Material
	{
	public:
		Material() = default;

		void setShader(std::shared_ptr<Graphics::IShader> shader) { this->m_shader = shader; }
		void setTexture(std::shared_ptr<Graphics::ITexture> texture) { this->m_texture = texture; }
		void setProperties(const MaterialProperties &props) { this->m_props = props; }

		void bind();
		void unbind();

		Graphics::IShader *getShader() { return this->m_shader.get(); }
		const MaterialProperties &getProperties() const { return this->m_props; }

	private:
		std::shared_ptr<Graphics::IShader> m_shader;
		std::shared_ptr<Graphics::ITexture> m_texture;
		MaterialProperties m_props;
	};

} // namespace GE
