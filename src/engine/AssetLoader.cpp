#include "AssetLoader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb/stb_image.h"

#include <fstream>
#include <sstream>

namespace GE
{

	// --- TextureLoader ---

	std::shared_ptr<Graphics::ITexture> TextureLoader::load(const std::string &path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		if (!data)
		{
			std::cerr << "[TextureLoader] Failed to load: " << path << " (" << stbi_failure_reason() << ")\n";
			return nullptr;
		}

		auto tex = Graphics::Factory::createTexture();
		Graphics::TextureFormat fmt = (channels == 4) ? Graphics::TextureFormat::RGBA8 : Graphics::TextureFormat::RGB8;

		if (!tex->create(width, height, fmt, data))
		{
			std::cerr << "[TextureLoader] Failed to create texture from: " << path << "\n";
			stbi_image_free(data);
			return nullptr;
		}

		tex->setFilter(Graphics::TextureFilter::LinearMipmap, Graphics::TextureFilter::Linear);
		tex->setWrap(Graphics::TextureWrap::Repeat, Graphics::TextureWrap::Repeat);

		stbi_image_free(data);
		std::cout << "[TextureLoader] Loaded " << path << " (" << width << "x" << height << ", " << channels << "ch)\n";
		return tex;
	}

	// --- ModelLoader (OBJ) ---

	std::vector<ModelLoader::LoadedMesh> ModelLoader::loadOBJ(const std::string &path)
	{
		std::vector<LoadedMesh> meshes;

		std::ifstream file(path);
		if (!file.is_open())
		{
			std::cerr << "[ModelLoader] Cannot open: " << path << "\n";
			return meshes;
		}

		std::vector<Vec3> positions;
		std::vector<Vec3> normals;
		std::vector<Vec2> texCoords;

		LoadedMesh currentMesh;
		currentMesh.name = "default";

		// Map from face vertex key to index (for deduplication)
		std::unordered_map<std::string, uint32_t> vertexMap;

		std::string line;
		while (std::getline(file, line))
		{
			if (line.empty() || line[0] == '#')
				continue;

			std::istringstream iss(line);
			std::string token;
			iss >> token;

			if (token == "o" || token == "g")
			{
				// New object/group — flush current mesh if it has data
				if (!currentMesh.vertices.empty())
				{
					meshes.push_back(std::move(currentMesh));
					currentMesh = LoadedMesh{};
					vertexMap.clear();
				}
				iss >> currentMesh.name;
			}
			else if (token == "v")
			{
				Vec3 p;
				iss >> p.x >> p.y >> p.z;
				positions.push_back(p);
			}
			else if (token == "vn")
			{
				Vec3 n;
				iss >> n.x >> n.y >> n.z;
				normals.push_back(n);
			}
			else if (token == "vt")
			{
				Vec2 t;
				iss >> t.x >> t.y;
				texCoords.push_back(t);
			}
			else if (token == "f")
			{
				// Parse face — support v, v/vt, v/vt/vn, v//vn
				std::vector<uint32_t> faceIndices;
				std::string vertStr;
				while (iss >> vertStr)
				{
					auto it = vertexMap.find(vertStr);
					if (it != vertexMap.end())
					{
						faceIndices.push_back(it->second);
						continue;
					}

					// Parse v/vt/vn
					Vertex vert{};
					vert.color = {0.8f, 0.8f, 0.8f, 1.0f};

					int vi = 0, ti = 0, ni = 0;
					if (sscanf(vertStr.c_str(), "%d/%d/%d", &vi, &ti, &ni) == 3)
					{
						vert.position = positions[vi - 1];
						vert.texCoord = texCoords[ti - 1];
						vert.normal = normals[ni - 1];
					}
					else if (sscanf(vertStr.c_str(), "%d//%d", &vi, &ni) == 2)
					{
						vert.position = positions[vi - 1];
						vert.normal = normals[ni - 1];
					}
					else if (sscanf(vertStr.c_str(), "%d/%d", &vi, &ti) == 2)
					{
						vert.position = positions[vi - 1];
						vert.texCoord = texCoords[ti - 1];
					}
					else if (sscanf(vertStr.c_str(), "%d", &vi) == 1)
					{
						vert.position = positions[vi - 1];
					}

					uint32_t idx = (uint32_t)currentMesh.vertices.size();
					currentMesh.vertices.push_back(vert);
					vertexMap[vertStr] = idx;
					faceIndices.push_back(idx);
				}

				// Triangulate (fan from first vertex)
				for (size_t i = 2; i < faceIndices.size(); ++i)
				{
					currentMesh.indices.push_back(faceIndices[0]);
					currentMesh.indices.push_back(faceIndices[i - 1]);
					currentMesh.indices.push_back(faceIndices[i]);
				}
			}
		}

		// Flush last mesh
		if (!currentMesh.vertices.empty())
			meshes.push_back(std::move(currentMesh));

		size_t totalVerts = 0, totalTris = 0;
		for (auto &m : meshes)
		{
			totalVerts += m.vertices.size();
			totalTris += m.indices.size() / 3;
		}
		std::cout << "[ModelLoader] Loaded " << path << " (" << meshes.size() << " meshes, "
				  << totalVerts << " verts, " << totalTris << " tris)\n";

		return meshes;
	}

} // namespace GE
