#pragma once
#include <atomic>
#include <queue>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

#include "../Asset/Factory/AssetFactory.h"
#include "../GameObject/Interface/IGameObject.h"
#include "../../Core/Object/Field/Interface/IFieldContext.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    class AutoMcpEngineAccess;
}

namespace NanamiEngine::Scene
{
    class Scene final : public Module::Object::IObject
    {
        friend class ::NanamiEngine::Core::Application::AutoMcp::AutoMcpEngineAccess;

    public:
        /** @brief .scene をデシリアライズした中間結果。InitGameObject はまだ呼ばれていない */
        struct DeserializedContent
        {
            std::string name = "Scene";
            std::vector<std::shared_ptr<Module::GameObject::IGameObject>> gameObjects;
            std::vector<std::weak_ptr<Core::Object::IFieldContext>> pendingFieldContexts;
        };

        /** @brief デシリアライズ済みのルート GameObject 数。ワーカーが書き、メインスレッドが読む */
        struct DeserializeProgress
        {
            std::atomic<int> total{0};
            std::atomic<int> done {0};
        };

        /**
         * @brief .scene を読むだけで InitGameObject は呼ばない。ワーカースレッドから呼べる
         * @warning 例外が出ても outContent には途中まで積まれた GameObject が残る。
         *          GameObject の破棄はメインスレッドで行う必要があるため、
         *          メインスレッドが所有する変数を渡すこと
         */
        static void Deserialize(const std::string& filePath, DeserializedContent& outContent, DeserializeProgress* progress);

        explicit Scene(const std::string& filePath = "");
        /** @brief Deserialize の結果から組み立てる。メインスレッド専用 */
        Scene(const std::string& filePath, DeserializedContent&& content);
        ~Scene() override;
        [[nodiscard]] std::string Name()    const           { return name_; }
        [[nodiscard]] const Guid& GetGuid() const override  { return guid_;    }
        [[nodiscard]] const std::string& FilePath() const   { return filePath_; }

        void CopiedInit(const std::string& contentPath);
        void AddGameObject(const std::weak_ptr<Module::GameObject::IGameObject>& add);
        void RemoveImplementAllGameObject();
        bool TryOnRemoveGameObject(const std::weak_ptr<Module::GameObject::IGameObject>& remove);
        void RemoveGameObject(const std::weak_ptr<Module::GameObject::IGameObject>& remove);
        void OnUpdatePushedContents();
        void OnDrawGui(const std::function<void(Scene*)>& onRemoveScene, Core::FileSystem::EditorDraggingHand& fileDraggingHand, const std::string& searchFilter = "");
        void OnDrawGui() override { }
        void OnDrawFileDropGui(Core::FileSystem::EditorDraggingHand& fileDraggingHand);
        void OnSave();
        [[nodiscard]] std::shared_ptr<Module::GameObject::IGameObject> CatchGameObject(const Guid& id) const;
        /** @brief 子孫も含めた全 GameObject に action を 1 回ずつ呼ぶ */
        void ForEachGameObject(const std::function<void(const std::shared_ptr<Module::GameObject::IGameObject>&)>& action) const;

    private:
        void AdoptDeserializedContent(DeserializedContent&& content);

        std::string filePath_ = "Assets/Scene/SampleScene.scene";
        std::string name_ = "Scene";
        Guid guid_;

        std::queue<std::weak_ptr<Module::GameObject::IGameObject>> addGameObjectQueue_;
        std::queue<std::weak_ptr<Module::GameObject::IGameObject>> removeGameObjectQueue_;
        std::unordered_map<Guid, std::weak_ptr<Module::GameObject::IGameObject>, GuidHash> gameObjects_;
    };
}