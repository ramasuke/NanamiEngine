#include "GameObject.h"

#include "../../../Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../CopiedPrefabGameObject/CopiedPrefabGameObject.h"
#include "../../../../Core/Application/Window/Main/Game/GameWindow.h"
#include "../SceneGameObject/SceneGameObject.h"

std::weak_ptr<Scene::SceneGameObject> Scene::GameObject::Instantiate()
{
    const auto gameObject = std::make_shared<SceneGameObject>();
    std::weak_ptr weakGameObject = gameObject;
    gameObject->InitGameObject(std::weak_ptr<Module::GameObject::IGameObject>(), gameObject);
    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(gameObject);
    return weakGameObject;
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(Asset::PrefabGameObjectFile& prefab,
    const std::shared_ptr<Module::GameObject::IGameObject>& parent)
{
    auto copiedPrefab = prefab.Content()->CopyForInstantiate();
    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    copiedPrefab->Transform().SetParent(parent);
    return copiedPrefab;       
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(Asset::PrefabGameObjectFile& prefab,
    const glm::vec3 position)
{
    auto copiedPrefab = prefab.Content()->CopyForInstantiate();
    copiedPrefab->Transform().SetWorldPos(position);
    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    return copiedPrefab;
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(std::shared_ptr<Asset::PrefabGameObjectFile> prefab, const glm::vec3 position)
{
    auto copiedPrefab = prefab->Content()->CopyForInstantiate();
    copiedPrefab->Transform().SetWorldPos(position);
    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    return copiedPrefab;
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(
    const std::shared_ptr<Module::GameObject::IGameObject>& gameObject, const glm::vec3 position)
{
    auto copiedPrefab = gameObject->CopyForInstantiate();
    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    copiedPrefab->Transform().SetWorldPos(position);
    return copiedPrefab;
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(
    Module::GameObject::IGameObject& gameObject, const std::shared_ptr<Module::GameObject::IGameObject>& parent)
{
    auto copiedPrefab = gameObject.CopyForInstantiate();
    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    copiedPrefab->Transform().SetParent(parent);
    return copiedPrefab;
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(
    Module::GameObject::IGameObject& gameObject, const glm::vec3 position)
{
    auto copiedPrefab = gameObject.CopyForInstantiate();
    copiedPrefab->Transform().SetWorldPos(position);
    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    return copiedPrefab;
}
