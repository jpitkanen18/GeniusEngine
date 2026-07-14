#include "Scene.hpp"

namespace GE
{

	std::shared_ptr<SceneNode> Scene::createNode(const std::string &name)
	{
		auto node = std::make_shared<SceneNode>();
		node->name = name;
		this->m_nodes.push_back(node);
		return node;
	}

	void Scene::addNode(std::shared_ptr<SceneNode> node)
	{
		this->m_nodes.push_back(node);
	}

	void Scene::removeNode(const std::string &name)
	{
		this->m_nodes.erase(
			std::remove_if(this->m_nodes.begin(), this->m_nodes.end(),
						   [&](const auto &n)
						   { return n->name == name; }),
			this->m_nodes.end());
	}

} // namespace GE
