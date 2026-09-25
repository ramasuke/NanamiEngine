#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <memory>

#include "../glm/fwd.hpp"
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
    NANAMI_API std::weak_ptr<SceneGameObject> Instantiate();
    NANAMI_API std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Module::Asset::PrefabGameObjectFile& prefab, std::shared_ptr<Module::GameObject::IGameObject> parent = nullptr);
    NANAMI_API std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Module::Asset::PrefabGameObjectFile& prefab, const glm::vec3 position);
    NANAMI_API std::weak_ptr<Module::GameObject::IGameObject> Instantiate(std::shared_ptr<Module::Asset::PrefabGameObjectFile> prefab, const glm::vec3 position);
    NANAMI_API std::weak_ptr<Module::GameObject::IGameObject> Instantiate(const std::shared_ptr<Module::GameObject::IGameObject>&, const glm::vec3 position);
    NANAMI_API std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Module::GameObject::IGameObject& gameObject, const std::shared_ptr<Module::GameObject::IGameObject>& parent);
    NANAMI_API std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Module::GameObject::IGameObject& gameObject, const glm::vec3 position);
    NANAMI_API std::weak_ptr<Module::GameObject::IGameObject> Instantiate(std::shared_ptr<Asset::PrefabGameObjectFile> prefab, const glm::vec3 position, glm::quat rotation);
    NANAMI_API std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Asset::PrefabGameObjectFile& gameObject, const glm::vec3 position, glm::quat rotation);
    NANAMI_API std::weak_ptr<Module::GameObject::IGameObject> Instantiate(Module::GameObject::IGameObject& gameObject, const glm::vec3 position, glm::quat rotation);
}
