#include "Scene.h"
#include <algorithm>
#include <cctype>
#include <ranges>
#include <fstream>
#include <string_view>

#include "../Asset/PrefabGameObject/PrefabGameObjectFile.h"
#include "../Asset/Preload/Engine_Asset_AssetPreloader.h"
#include "../GameObject/PrefabGameObject/PrefabGameObject.h"
#include "../Log/NanamiEngine_Module_Log.h"
#include "../../Core/Application/ApplicationBase.h"
#include "../../Core/Application/LifeCycle/ApplicationLifeCycle.h"
#include "../Serialization/Engine_Module_Serialization.h"
#include "cereal/archives/json.hpp"
#include "GameObject/CopiedPrefabGameObject/CopiedPrefabGameObject.h"
#include "GameObject/Helper/GameObject.h"
#include "GameObject/SceneGameObject/SceneGameObject.h"

namespace
{
    /** @brief haystackにneedleが含まれるか大文字小文字を無視して判定する */
    bool ContainsCaseInsensitive(const std::string_view haystack, const std::string_view needle)
    {
        if (needle.empty())
            return true;

        const auto equalsIgnoreCase = [](const char lhs, const char rhs)
        {
            return std::tolower(static_cast<unsigned char>(lhs)) ==
                   std::tolower(static_cast<unsigned char>(rhs));
        };

        return !std::ranges::search(haystack, needle, equalsIgnoreCase).empty();
    }
}

void Scene::Scene::Deserialize(
    const std::string& filePath,
    DeserializedContent& outContent,
    DeserializeProgress* progress)
{
    // FIELD の初期化待ちは共有キューに積まず outContent に貯める。
    // 共有キューに積むと、まだ ObjectRegistry に登録されていない GameObject を
    // メインスレッドが解決してしまい、参照が null のまま確定する
    const Core::Application::FieldInitStagingScope stagingScope(outContent.pendingFieldContexts);

    // 未作成のファイルは空の Scene として扱う（新規作成 → Save のフローで使う）。
    // 破損している場合は DeserializeException が投げられる
    NanamiEngine::Module::Serialization::LoadJsonFileIfExists(filePath, [&outContent, progress](cereal::JSONInputArchive& archive)
    {
        archive(cereal::make_nvp("name", outContent.name));

        std::size_t count = 0;
        archive(cereal::make_nvp("gameObjectCount", count));
        if (progress)
            progress->total.store(static_cast<int>(count), std::memory_order_release);

        outContent.gameObjects.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            std::shared_ptr<Module::GameObject::IGameObject> gameObject;
            archive(cereal::make_nvp("gameObject_" + std::to_string(i), gameObject));
            if (gameObject)
                outContent.gameObjects.push_back(std::move(gameObject));

            if (progress)
                progress->done.store(static_cast<int>(i) + 1, std::memory_order_release);
        }
    });
}

Scene::Scene::Scene(const std::string& filePath)
{
    filePath_ = filePath;
    // 中のオブジェクトが初めて描画するときに同期ロードで止まらないよう、参照先の読み込みを先に要求しておく
    Module::Asset::AssetPreloader::RequestForScene(filePath_);

    DeserializedContent content;
    Deserialize(filePath_, content, nullptr);
    AdoptDeserializedContent(std::move(content));
}

Scene::Scene::Scene(const std::string& filePath, DeserializedContent&& content)
{
    filePath_ = filePath;
    AdoptDeserializedContent(std::move(content));
}

void Scene::Scene::AdoptDeserializedContent(
    DeserializedContent&& content)
{
    name_ = std::move(content.name);

    // デシリアライズ中に貯めた FIELD を共有キューへ戻す。
    // InitGameObject より先に積んでおくと、同期読み込みのときと順序が揃う
    Core::Application::ApplicationBase::ApplicationLifeCycle().AddStagedFieldInittables(content.pendingFieldContexts);

    for (const auto& gameObject : content.gameObjects)
    {
        gameObjects_[gameObject->GetGuid()] = gameObject;
        gameObject->InitGameObject(std::weak_ptr<Module::GameObject::IGameObject>(), gameObject);
    }
    content.gameObjects.clear();
}

Scene::Scene::~Scene()
{
    //TODO: replace RemoveImplementAllGameObject()
    for (auto& weakGameObject : gameObjects_ | std::views::values)
    {
        if (const auto gameObject = weakGameObject.lock())
        {
            gameObject->ImplementDestroy();
        }
    }
}

void Scene::Scene::CopiedInit(
    const std::string& contentPath)
{
    for (auto& weakGameObject : gameObjects_ | std::views::values)
    {
        auto gameObject = weakGameObject.lock();
        weakGameObject.lock()->InitForCopied(
            gameObject,
            gameObject->IsEnable(),
            gameObject->Name(),
            gameObject->Mark(),
            gameObject->Components(),
            gameObject->Transform());
    }
    filePath_ = contentPath;
}

void Scene::Scene::AddGameObject(
    const std::weak_ptr<Module::GameObject::IGameObject>& add)
{
    Core::Application::ApplicationBase::ApplicationLifeCycle().OnUpdateFieldInittables();
    addGameObjectQueue_.push(add);
}

void Scene::Scene::RemoveImplementAllGameObject()
{
    std::vector<Guid> toRemove;

    for (auto& [guid, weakGameObject] : gameObjects_)
    {
        if (const auto remove = weakGameObject.lock())
        {
            remove->ImplementDestroy();
            toRemove.push_back(guid);
        }
    }

    for (const auto& guid : toRemove)
    {
        gameObjects_.erase(guid);
    }
}

bool Scene::Scene::TryOnRemoveGameObject(const std::weak_ptr<Module::GameObject::IGameObject>& remove)
{
    if (const auto shared = remove.lock())
    {
        if (const auto it = gameObjects_.find(shared->GetGuid()); it != gameObjects_.end())
        {
            removeGameObjectQueue_.push(remove);
            return true;
        }
    }
    return false;
}

void Scene::Scene::RemoveGameObject(const std::weak_ptr<Module::GameObject::IGameObject>& remove)
{
    removeGameObjectQueue_.push(remove);
}

void Scene::Scene::OnUpdatePushedContents()
{
    while (!addGameObjectQueue_.empty())
    {
        const std::weak_ptr<Module::GameObject::IGameObject>& addWeak = addGameObjectQueue_.front();
        if (const std::shared_ptr<Module::GameObject::IGameObject> add = addWeak.lock())
        {
            gameObjects_[add->GetGuid()] = addWeak;
        }
        addGameObjectQueue_.pop();
    }

    while (!removeGameObjectQueue_.empty())
    {
        const std::weak_ptr<Module::GameObject::IGameObject>& removeWeak = removeGameObjectQueue_.front();
        if (const std::shared_ptr<Module::GameObject::IGameObject> remove = removeWeak.lock())
        {
            gameObjects_.erase(remove->GetGuid());
        }
        removeGameObjectQueue_.pop();
    }
}

void Scene::Scene::OnDrawGui(const std::function<void(Scene*)>& onRemoveScene, Core::FileSystem::EditorDraggingHand& fileDraggingHand, const std::string& searchFilter)
{
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        ImGui::OpenPopup(("SceneHeaderPopup##" + guid_.Value()).c_str());
    }

    if (ImGui::BeginPopup(("SceneHeaderPopup##" + guid_.Value()).c_str()))
    {
        if (ImGui::MenuItem(("Add GameObject##" + guid_.Value()).c_str()))
        {
            AddGameObject(GameObject::Instantiate());
        }
        if (ImGui::MenuItem(("Remove Scene##" + guid_.Value()).c_str()))
        {
            onRemoveScene(this);   
        }
        ImGui::EndPopup();
    }

    OnDrawFileDropGui(fileDraggingHand);

    // 検索中は階層を無視して、子孫まで含めた全GameObjectから名前がマッチするものをフラットに一覧表示する
    if (!searchFilter.empty())
    {
        ForEachGameObject([&searchFilter](const std::shared_ptr<Module::GameObject::IGameObject>& gameObject)
        {
            if (gameObject && ContainsCaseInsensitive(gameObject->Name(), searchFilter))
                gameObject->OnDrawTreeGui(false);
        });
        return;
    }

    // 通常のGameObject描画
    for (const std::weak_ptr<Module::GameObject::IGameObject>& weakGameObject : gameObjects_ | std::views::values)
    {
        if (const std::shared_ptr<Module::GameObject::IGameObject> gameObject = weakGameObject.lock())
        {
            if (gameObject->Transform().GetParent() != nullptr)
                continue;

            gameObject->OnDrawTreeGui();
        }
    }
}

void Scene::Scene::OnDrawFileDropGui(Core::FileSystem::EditorDraggingHand& fileDraggingHand)
{
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(Core::FileSystem::EDITOR_DRAGGING_ITEM_PAYLOAD_TYPE))
        {
            const auto draggingGuid = fileDraggingHand.TakeDraggingItemGuid();
            if (const auto prefabGameObjectFile = Core::Application::ApplicationBase::ObjectRegistry().Catch<Module::Asset::PrefabGameObjectFile>(draggingGuid.value()); !prefabGameObjectFile.expired())
            {
                // .prefab の読み込みに失敗している場合は Content() が null
                if (const auto content = prefabGameObjectFile.lock()->Content())
                    AddGameObject(content->CopyForEditor());
                else
                    NanamiEngine::Module::LogError("Scene: Prefab の内容が読み込まれていないため配置できません: " + prefabGameObjectFile.lock()->GetContentPath());
            }
            if (const auto sceneGameObject = Core::Application::ApplicationBase::ObjectRegistry().Catch<Module::GameObject::IGameObject>(draggingGuid.value()); !sceneGameObject.expired())
            {
                AddGameObject(sceneGameObject);
                sceneGameObject.lock()->Transform().SetParent(std::weak_ptr<Module::GameObject::IGameObject>());
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void Scene::Scene::Deserialize(
    std::istream& stream,
    const std::string& label,
    DeserializedContent& outContent,
    DeserializeProgress* progress)
{
    const Core::Application::FieldInitStagingScope stagingScope(outContent.pendingFieldContexts);
    NanamiEngine::Module::Serialization::Detail::ReadWith<cereal::JSONInputArchive>(stream, label, [&outContent, progress](cereal::JSONInputArchive& archive)
    {
        archive(cereal::make_nvp("name", outContent.name));

        std::size_t count = 0;
        archive(cereal::make_nvp("gameObjectCount", count));
        if (progress)
            progress->total.store(static_cast<int>(count), std::memory_order_release);

        outContent.gameObjects.reserve(count);
        for (std::size_t i = 0; i < count; ++i)
        {
            std::shared_ptr<Module::GameObject::IGameObject> gameObject;
            archive(cereal::make_nvp("gameObject_" + std::to_string(i), gameObject));
            if (gameObject)
                outContent.gameObjects.push_back(std::move(gameObject));

            if (progress)
                progress->done.store(static_cast<int>(i) + 1, std::memory_order_release);
        }
    });
}

void Scene::Scene::OnSave()
{
    std::ofstream ofStream(filePath_);
    if (!ofStream.is_open())
        return;
    SaveTo(ofStream);
}

void Scene::Scene::SaveTo(std::ostream& stream)
{
    cereal::JSONOutputArchive archive(stream);
    archive(cereal::make_nvp("name", name_));

    std::vector<std::shared_ptr<Module::GameObject::IGameObject>> rootObjects;
    for (const auto& weakGameObject : gameObjects_ | std::views::values)
    {
        if (auto gameObject = weakGameObject.lock())
        {
            if (!gameObject->Transform().GetParent())
            {
                rootObjects.push_back(gameObject);
            }
        }
    }

    std::size_t count = rootObjects.size();
    archive(cereal::make_nvp("gameObjectCount", count));

    for (std::size_t i = 0; i < count; ++i)
    {
        archive(cereal::make_nvp("gameObject_" + std::to_string(i), rootObjects[i]));
    }
}

std::shared_ptr<GameObject::IGameObject> Scene::Scene::CatchGameObject(
    const Guid& id) const
{
    if (const auto it = gameObjects_.find(id); it != gameObjects_.end()) {
        return it->second.lock();
    }
    return nullptr;
}

void Scene::Scene::ForEachGameObject(
    const std::function<void(const std::shared_ptr<Module::GameObject::IGameObject>&)>& action) const
{
    // 親を持つ GameObject も gameObjects_ に残っていることがあるので、ルートから辿って重複を避ける
    for (const std::weak_ptr<Module::GameObject::IGameObject>& weakGameObject : gameObjects_ | std::views::values)
    {
        const std::shared_ptr<Module::GameObject::IGameObject> rootGameObject = weakGameObject.lock();
        if (!rootGameObject || rootGameObject->Transform().GetParent() != nullptr)
            continue;

        action(rootGameObject);
        for (const auto& child : rootGameObject->Transform().GetAllChildren())
        {
            action(child);
        }
    }
}
