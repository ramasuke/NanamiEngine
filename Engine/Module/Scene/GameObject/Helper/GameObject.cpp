#include "GameObject.h"

#include "../../../Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../CopiedPrefabGameObject/CopiedPrefabGameObject.h"
#include "../../../../Core/Application/Window/Main/Game/GameWindow.h"
#include "../SceneGameObject/SceneGameObject.h"
#include "../../../Log/NanamiEngine_Module_Log.h"

namespace
{
    /** Prefab の内容が読み込めていない（.prefab が壊れている等）場合は null を返し、エラーを記録する */
    std::shared_ptr<GameObject::PrefabGameObject> CatchPrefabContentForInstantiate(Asset::PrefabGameObjectFile& prefab)
    {
        const auto content = prefab.Content();
        if (!content)
            NanamiEngine::Module::LogError("Instantiate: Prefab の内容が読み込まれていないため生成できません: " + prefab.GetContentPath());
        return content;
    }
}

std::weak_ptr<Scene::SceneGameObject> Scene::GameObject::Instantiate()
{
    const auto gameObject = std::make_shared<SceneGameObject>();
    std::weak_ptr weakGameObject = gameObject;
    gameObject->InitGameObject(std::weak_ptr<Module::GameObject::IGameObject>(), gameObject);
    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(gameObject);
    return weakGameObject;
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(Asset::PrefabGameObjectFile& prefab,
    std::shared_ptr<Module::GameObject::IGameObject> parent)
{
    const auto content = CatchPrefabContentForInstantiate(prefab);
    if (!content)
        return {};

    auto copiedPrefab = content->CopyForInstantiate();
    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    copiedPrefab->Transform().SetParent(parent);
    return copiedPrefab;
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(Asset::PrefabGameObjectFile& prefab,
    const glm::vec3 position)
{
    const auto content = CatchPrefabContentForInstantiate(prefab);
    if (!content)
        return {};

    auto copiedPrefab = content->CopyForInstantiate();
    copiedPrefab->Transform().SetWorldPos(position);
    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    return copiedPrefab;
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(std::shared_ptr<Asset::PrefabGameObjectFile> prefab, const glm::vec3 position)
{
    const auto content = CatchPrefabContentForInstantiate(*prefab);
    if (!content)
        return {};

    auto copiedPrefab = content->CopyForInstantiate();
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

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(
    std::shared_ptr<Asset::PrefabGameObjectFile> prefab, const glm::vec3 position, glm::quat rotation)
{
    const auto content = CatchPrefabContentForInstantiate(*prefab);
    if (!content)
        return {};

    auto copiedPrefab = content->CopyForInstantiate();
    copiedPrefab->Transform().SetWorldPos(position);
    copiedPrefab->Transform().SetWorldRot(rotation);

    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    return copiedPrefab;
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(
    Asset::PrefabGameObjectFile& gameObject,
    const glm::vec3 position,
    const glm::quat rotation)
{
    const auto content = CatchPrefabContentForInstantiate(gameObject);
    if (!content)
        return {};

    auto copiedPrefab = content->CopyForInstantiate();
    copiedPrefab->Transform().SetWorldPos(position);
    copiedPrefab->Transform().SetWorldRot(rotation);

    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    return copiedPrefab;
}

std::weak_ptr<GameObject::IGameObject> Scene::GameObject::Instantiate(
    Module::GameObject::IGameObject& gameObject, const glm::vec3 position, const glm::quat rotation)
{
    auto copiedPrefab = gameObject.CopyForInstantiate();
    copiedPrefab->Transform().SetWorldPos(position);
    copiedPrefab->Transform().SetWorldRot(rotation);

    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    return copiedPrefab;   
}

std::weak_ptr<Module::GameObject::IGameObject> Instantiate(
    const std::shared_ptr<GameObject::IGameObject>& gameObject,
    const glm::vec3& position,
    const glm::quat& rotation)
{
    auto copiedPrefab = gameObject->CopyForInstantiate();
    copiedPrefab->Transform().SetWorldPos(position);
    copiedPrefab->Transform().SetWorldRot(rotation);

    Core::Application::ApplicationBase::GameWindow()->MainScene().AddGameObject(copiedPrefab);
    return copiedPrefab;
}
