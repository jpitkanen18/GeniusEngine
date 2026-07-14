#pragma once
// GeniusEngine GAPI Factory
// Creates the right backend objects based on active backend

#include "GAPI.hpp"
#include "Shader.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"

namespace GE::Graphics
{

	class Factory
	{
	public:
		static void setBackend(Backend backend) { s_backend = backend; }
		static Backend getBackend() { return s_backend; }

		static std::shared_ptr<IVertexBuffer> createVertexBuffer();
		static std::shared_ptr<IIndexBuffer> createIndexBuffer();
		static std::shared_ptr<IVertexArray> createVertexArray();
		static std::shared_ptr<IShader> createShader();
		static std::shared_ptr<ITexture> createTexture();

	private:
		static Backend s_backend;
	};

} // namespace GE::Graphics
