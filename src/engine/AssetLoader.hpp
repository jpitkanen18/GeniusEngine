#pragma once
// GeniusEngine - Asset loading utilities
// Shader, texture, and model loaders

#include "../core/Types.hpp"
#include "../graphics/Factory.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace GE
{

	class ShaderLoader
	{
	public:
		// Load a shader pair by name (e.g. "default" loads default.vert + default.frag)
		// Automatically picks the right backend folder (opengl/, metal/, vulkan/)
		static std::shared_ptr<Graphics::IShader> load(const std::string &name,
													   const std::string &shadersDir = "src/shaders")
		{
			auto shader = Graphics::Factory::createShader();
			auto backend = Graphics::Factory::getBackend();

			std::string subdir;
			std::string vertExt, fragExt;

			switch (backend)
			{
			case Graphics::Backend::Metal:
				subdir = "metal";
				// Metal uses a single .metal file
				{
					std::string path = shadersDir + "/" + subdir + "/" + name + ".metal";
					std::string src = readFile(path);
					if (src.empty())
					{
						std::cerr << "[ShaderLoader] Failed to load: " << path << "\n";
						return nullptr;
					}
					if (!shader->loadFromSource(src, ""))
						return nullptr;
				}
				return shader;

			case Graphics::Backend::Vulkan:
				subdir = "vulkan";
				break;

			case Graphics::Backend::OpenGL:
			default:
				subdir = "opengl";
				break;
			}

			std::string vertPath = shadersDir + "/" + subdir + "/" + name + ".vert";
			std::string fragPath = shadersDir + "/" + subdir + "/" + name + ".frag";

			// Try loadFromFile first (backend can do its own loading, e.g. SPIR-V compile)
			if (shader->loadFromFile(vertPath, fragPath))
				return shader;

			// Fallback: read source and compile
			std::string vertSrc = readFile(vertPath);
			std::string fragSrc = readFile(fragPath);
			if (vertSrc.empty() || fragSrc.empty())
			{
				std::cerr << "[ShaderLoader] Failed to load shader '" << name << "' from " << subdir << "/\n";
				return nullptr;
			}
			if (!shader->loadFromSource(vertSrc, fragSrc))
				return nullptr;
			return shader;
		}

		// Load a custom shader from an app-specific directory.
		// Expects the same backend subfolder structure (opengl/, metal/, vulkan/).
		// Usage: ShaderLoader::loadCustom("myeffect", "src/app/shaders")
		static std::shared_ptr<Graphics::IShader> loadCustom(const std::string &name,
															 const std::string &appShadersDir)
		{
			return load(name, appShadersDir);
		}

		static std::string readFile(const std::string &path)
		{
			std::ifstream file(path);
			if (!file.is_open())
			{
				std::cerr << "[ShaderLoader] Cannot open: " << path << "\n";
				return "";
			}
			std::stringstream ss;
			ss << file.rdbuf();
			return ss.str();
		}
	};

	class TextureLoader
	{
	public:
		// Load a texture from an image file (PNG, JPG, BMP, TGA)
		static std::shared_ptr<Graphics::ITexture> load(const std::string &path);
	};

	class ModelLoader
	{
	public:
		// Load an OBJ model. Returns a vector of meshes (one per group/object).
		struct LoadedMesh
		{
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			std::string name;
		};

		static std::vector<LoadedMesh> loadOBJ(const std::string &path);
	};

} // namespace GE
