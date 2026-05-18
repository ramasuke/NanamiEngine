#pragma once
#include <queue>
#include <unordered_map>
#include <memory>
#include <string>

#include "../Asset/Factory/AssetFactory.h"
#include "../GameObject/Interface/IGameObject.h"

namespace NanamiEngine::Scene
{
    class Scene final : public Module::Object::IObject
    {
    public:
        explicit Scene(const std::string& filePath = "");
        ~Scene() override;
        [[nodiscard]] std::string Name()    const           { return name_; }
        [[nodiscard]] const Guid& GetGuid() const override  { return guid_;    }

        void CopiedInit();
        void AddGameObject(const std::weak_ptr<Module::GameObject::IGameObject>& add);
        void RemoveImplementAllGameObject();
        bool TryOnRemoveGameObject(const std::weak_ptr<Module::GameObject::IGameObject>& remove);
        void RemoveGameObject(const std::weak_ptr<Module::GameObject::IGameObject>& remove);
        void OnUpdatePushedContents();
        void OnDrawGui(const std::function<void(Scene*)>& onRemoveScene, Core::FileSystem::EditorDraggingHand& fileDraggingHand);
        void OnDrawGui() override { }
        void OnDrawFileDropGui(Core::FileSystem::EditorDraggingHand& fileDraggingHand);
        void OnSave();
        [[nodiscard]] std::shared_ptr<Module::GameObject::IGameObject> CatchGameObject(const Guid& id) const;

    private:
        std::string filePath_ = "Assets/Scene/SampleScene.scene";
        std::string name_ = "Scene";
        Guid guid_;

        std::queue<std::weak_ptr<Module::GameObject::IGameObject>> addGameObjectQueue_;
        std::queue<std::weak_ptr<Module::GameObject::IGameObject>> removeGameObjectQueue_;
        std::unordered_map<Guid, std::weak_ptr<Module::GameObject::IGameObject>, GuidHash> gameObjects_;
    };
}