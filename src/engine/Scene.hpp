#pragma once
// GeniusEngine - Scene graph (backend-agnostic)

#include "../core/Types.hpp"
#include "Mesh.hpp"
#include "Material.hpp"

namespace GE
{

	struct SceneNode
	{
		std::string name;
		Transform transform;
		Mesh *mesh = nullptr;
		Material *material = nullptr;
		std::vector<std::shared_ptr<SceneNode>> children;
		bool visible = true;
	};

	class Scene
	{
	public:
		Scene() = default;

		std::shared_ptr<SceneNode> createNode(const std::string &name);
		void addNode(std::shared_ptr<SceneNode> node);
		void removeNode(const std::string &name);

		const std::vector<std::shared_ptr<SceneNode>> &getNodes() const { return this->m_nodes; }

		void clear() { this->m_nodes.clear(); }

	private:
		std::vector<std::shared_ptr<SceneNode>> m_nodes;
	};

} // namespace GE
