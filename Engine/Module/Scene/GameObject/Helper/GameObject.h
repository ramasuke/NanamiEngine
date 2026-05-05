#pragma once
#include <memory>

#include "fwd.hpp"
#include "../../Engine/Module/Namespace/EngineNamespace.h"

namespace NanamiEngine::Scene
{
    class SceneGameObject;
}

namespace NanamiEngine::Module::GameObject
{
    class IGameObject;
}

namespace NanamiEngine::Module::Asset
{
    class PrefabGameObjectFile;
}

namespace NanamiEngine::Scene::GameObject
{
    std::weak_ptr<SceneGameObject> Instantiate();
    std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Module::Asset::PrefabGameObjectFile& prefab, const std::shared_ptr<Module::GameObject::IGameObject>& parent);
    std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Module::Asset::PrefabGameObjectFile& prefab, const glm::vec3 position);
    std::weak_ptr<Module::GameObject::IGameObject> Instantiate(std::shared_ptr<Module::Asset::PrefabGameObjectFile> prefab, const glm::vec3 position);
    std::weak_ptr<Module::GameObject::IGameObject> Instantiate(const std::shared_ptr<Module::GameObject::IGameObject>&, const glm::vec3 position);
    std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Module::GameObject::IGameObject& gameObject, const std::shared_ptr<Module::GameObject::IGameObject>& parent);
    std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Module::GameObject::IGameObject& gameObject, const glm::vec3 position);
    std::weak_ptr<Module::GameObject::IGameObject> Instantiate(std::shared_ptr<Asset::PrefabGameObjectFile> prefab, const glm::vec3 position, glm::quat rotation);
    std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Asset::PrefabGameObjectFile& gameObject, const glm::vec3 position, glm::quat rotation);
    std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Module::GameObject::IGameObject& gameObject, const glm::vec3 position, glm::quat rotation);
}
