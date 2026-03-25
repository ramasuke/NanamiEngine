#pragma once
#include "DxLib.h"
#include "../MainWindowBase.h"
#include "../../../Editor/Camera/Free/Editor3DCamera.h"
#include "../Factory/MainWindowFactory.h"
#include "../../../../../Module/Scene/Scene.h"

namespace NanamiEngine::Core::MainWindow
{
    class GameWindow final : public MainWindowBase<Scene::Scene>
    {
    public:
        void AddContent(const std::shared_ptr<Scene::Scene>& content) override;
        [[nodiscard]] Scene::Scene& MainScene() const { return *mainScene_.lock(); }
        void ChangeMainScene(const std::shared_ptr<Scene::Scene>& scene);
        [[nodiscard]] std::shared_ptr<Scene::Scene> CatchScene(const Guid& guid) const;
        [[nodiscard]] VECTOR    GetCameraDxPosition      () const { return {editorCamera_.GetPosition().x, editorCamera_.GetPosition().y, editorCamera_.GetPosition().z}; }
        [[nodiscard]] glm::vec3 GetCameraPosition        () const { return editorCamera_.GetPosition(); }
        [[nodiscard]] glm::quat GetCameraRotation        () const { return editorCamera_.GetRotataion       (); }
        [[nodiscard]] glm::mat4 GetCameraViewMatrix      () const { return editorCamera_.GetViewMatrix      (); }
        [[nodiscard]] glm::mat4 GetCameraProjectionMatrix() const { return editorCamera_.GetProjectionMatrix(); }

        [[nodiscard]] bool IsPlayMode() const { return isPlayMode_; }
        [[nodiscard]] bool IsPlaying () const { return isPlaying_; }
        void RemoveGameObject(const std::weak_ptr<GameObject::IGameObject>& removeGameObject) const;
        bool TryReplaceGameObject(const Guid& replaceGameObjectGuid, const std::shared_ptr<GameObject::IGameObject>& newGameObject) const;

    private:
        [[nodiscard]] std::vector<std::shared_ptr<Scene::Scene>> Scenes() const;
        void OnUpdate()     override;
        void OnDrawGui(MainWindowDrawGuiContext context) override;
        void OnSave()       override;

    private:
        Component::Editor3DCamera editorCamera_;
        std::weak_ptr<Scene::Scene> mainScene_;
        bool isPlayMode_ = false;
        bool isPlaying_  = false;
    };

    REGISTER_MAIN_WINDOW(GameWindow)
}
