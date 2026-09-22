#pragma once
#include "DxLib.h"
#include "../MainWindowBase.h"
#include "../../../Editor/Camera/Free/Editor3DCamera.h"
#include "../Factory/MainWindowFactory.h"
#include "../../../../../Module/Scene/Scene.h"
#include "../../../../../Module/Scene/AsyncLoader/Engine_Scene_AsyncSceneLoader.h"

namespace NanamiEngine::Core::Application::AutoMcp
{
    class AutoMcpEngineAccess;
}

namespace NanamiEngine::Core::Application::Game
{
    class GameApplication;
}

namespace NanamiEngine::Core::MainWindow
{
    class GameWindow final : public MainWindowBase<Scene::Scene>
    {
        friend class ::NanamiEngine::Core::Application::AutoMcp::AutoMcpEngineAccess;
        friend class ::NanamiEngine::Core::Application::Game::GameApplication;

    public:
        explicit GameWindow();

        void AddContent(const std::shared_ptr<Scene::Scene>& content) override;
        /** @brief 外したシーンだけが使っていた画像・モデルは、読み込み中でないフレームで解放する */
        void RemoveContent(const std::shared_ptr<Scene::Scene>& content);
        void ChangeMainScene(const std::shared_ptr<Scene::Scene>& scene);
        [[nodiscard]] Scene::Scene& MainScene() const { return *mainScene_.lock(); }
        [[nodiscard]] std::shared_ptr<Scene::Scene> CatchScene(const Guid& guid) const;
        [[nodiscard]] VECTOR    GetCameraDxPosition      () const { return {editorCamera_.GetPosition().x, editorCamera_.GetPosition().y, editorCamera_.GetPosition().z}; }
        [[nodiscard]] glm::vec3 GetCameraPosition        () const { return editorCamera_.GetPosition(); }
        [[nodiscard]] glm::quat GetCameraRotation        () const { return editorCamera_.GetRotataion       (); }
        [[nodiscard]] glm::mat4 GetCameraViewMatrix      () const { return editorCamera_.GetViewMatrix      (); }
        [[nodiscard]] glm::mat4 GetCameraProjectionMatrix() const { return editorCamera_.GetProjectionMatrix(); }
        void SetCameraPosition(const glm::vec3& position) { editorCamera_.SetPosition(position); }
        void SetCameraRotation(const glm::quat& rotation) { editorCamera_.SetRotation(rotation); }

        [[nodiscard]] bool IsPlayMode() const { return isPlayMode_; }
        [[nodiscard]] bool IsPlaying () const { return isPlaying_; }
        [[nodiscard]] bool TryReplaceGameObject(const Guid& replaceGameObjectGuid, const std::shared_ptr<GameObject::IGameObject>& newGameObject) const;
        void RemoveGameObject(const std::weak_ptr<GameObject::IGameObject>& removeGameObject);

        /**
         * @brief シーンをワーカースレッドで読み込み始める。完了したフレームで自動的にメインシーンへ差し替わる
         * @return 既に別のシーンを読み込み中なら false（何もしない）
         */
        bool BeginLoadSceneAsync(const std::string& filePath);
        /** @brief 読み込み中のシーンを捨てる。捨てたシーンはメインシーンにならない。ワーカーの完了は待つ */
        void CancelSceneLoad();
        [[nodiscard]] bool IsSceneLoading() const;
        /** @brief 読み込み中のシーンのデシリアライズ進捗。総数が読めるまでは 0 */
        [[nodiscard]] float SceneLoadProgress01() const;
        /** @brief 直近の BeginLoadSceneAsync 以降に読み込みが失敗したか */
        [[nodiscard]] bool HasSceneLoadFailed() const;
        /** @brief BeginLoadSceneAsync で最後に読み込んだシーン */
        [[nodiscard]] std::weak_ptr<Scene::Scene> LastAsyncLoadedScene() const { return lastAsyncLoadedScene_; }

    private:
        [[nodiscard]] std::vector<std::shared_ptr<Scene::Scene>> Scenes() const;
        void Play();
        void Stop();
        /** @brief プレイを終了し、全シーンを破棄して初期シーンを読み直す */
        void End();
        void OnUpdate() override;
        void OnSave  () override;
        void OnDrawGui(MainWindowDrawGuiContext context) override;
        void DrawGameObjectMarks() const;
        /** @brief 非同期読み込みを 1 フレーム分進め、完了していればメインシーンへ差し替える */
        void UpdateAsyncSceneLoad();
        /** @brief シーンの差し替え・削除のあと、開いているどのシーンからも参照されない画像・モデルを解放する */
        void ReleaseUnusedAssetsIfPending();

        std::queue<std::weak_ptr<GameObject::IGameObject>> removeGameObjectQueue_;
        Scene::AsyncSceneLoader sceneLoader_;
        std::weak_ptr<Scene::Scene> lastAsyncLoadedScene_;
        Component::Editor3DCamera editorCamera_;
        std::weak_ptr<Scene::Scene> mainScene_;
        char hierarchySearchBuffer_[128] = {};
        bool isPlayMode_ = false;
        bool isPlaying_  = false;
        bool isAssetReleasePending_ = false;
    };

    REGISTER_MAIN_WINDOW(GameWindow, "Scene")
}
