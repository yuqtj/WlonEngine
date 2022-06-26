#pragma once

#include <vector>
#include <memory>
#include <string>

#include "Scene/GameObject.h"

class Transform;

class Scene
{
public:
	Scene() = default;

	inline void AddNode(GameObject* node) { m_GameObjects.emplace_back(node); };

	inline void SetNodes(std::vector<GameObject*>& nodes) { m_GameObjects = nodes; }

	inline void SetRootNode(GameObject* go) { m_RootNode = go; }
	inline GameObject* GetRootNode() { return m_RootNode; };

	GameObject* FindNode(const std::string& name);
protected:
private:
	std::vector<GameObject*> m_GameObjects;

	GameObject* m_RootNode;
};