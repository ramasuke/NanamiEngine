#include "GameWindow.h"

#include <memory>
#include <ranges>
#include <sstream>

#include "ImGuiHelper.h"
#include "../../../../../Module/Asset/Asset.h"
#include "../../../../../Module/Asset/Preload/Engine_Asset_AssetPreloader.h"
#include "../../../../../Module/GameObject/Transform/Transform.h"
#include "../../../../Coroutine/Scheduler/CoroutineScheduler.h"
#include "../../../../../Module/Exception/Engine_Module_Exception.h"
#include "../../../../../Module/Log/NanamiEngine_Module_Log.h"
#include "../../../Time/Time.h"
#include "../../../Configuration/Build/ApplicationConfiguration_Build.h"
#include "../../../Configuration/GameWindow/ApplicationConfiguration_GameWindow.h"

namespace NanamiEngine::Core::MainWindow
{
    GameWindow::GameWindow(): MainWindowBase(true)
    {
        
    }
    
    std::vector<std::shared_ptr<Scene::Scene>> GameWindow::Scenes() const
    {
        std::vector<std::shared_ptr<Scene::Scene>> result;
        std::ranges::copy(contents_ | std::views::values, std::back_inserter(result));
        return result;
    }

    void GameWindow::AddContent(const std::shared_ptr<Scene::Scene>& content)
    {
        MainWindowBase::AddContent(content);
        Application::ApplicationBase::ApplicationLifeCycle().OnUpdateFieldInittables();
    }
    
    void GameWindow::RemoveContent(const std::shared_ptr<Scene::Scene>& content)
    {
        MainWindowBase::RemoveContent(content);
        isAssetReleasePending_ = true;
    }

    void GameWindow::ChangeMainScene(const std::shared_ptr<Scene::Scene>& scene)
    {
        mainScene_ = scene;
        isAssetReleasePending_ = true;
        Application::ApplicationBase::ResetPhysics();

        //NOTE:
        constexpr int skipFrame = 60;
        for (int totalSkipFrame = 0; totalSkipFrame < skipFrame; totalSkipFrame++)
        {
            Time::SkipNextFrame();
        }
    }
    
    bool GameWindow::BeginLoadSceneAsync(const std::string& filePath)
    {
        if (!sceneLoader_.Begin(filePath))
        {
            Module::LogWarning("GameWindow: 既に別のシーンを読み込み中です: " + filePath);
            return false;
        }

        // 前回のシーンを今回の読み込み結果と取り違えないように
        lastAsyncLoadedScene_.reset();
        return true;
    }

    void GameWindow::CancelSceneLoad()
    {
        sceneLoader_.Cancel();
    }

    bool GameWindow::IsSceneLoading() const
    {
        return sceneLoader_.IsBusy();
    }

    float GameWindow::SceneLoadProgress01() const
    {
        return sceneLoader_.DeserializeProgress01();
    }

    bool GameWindow::HasSceneLoadFailed() const
    {
        return sceneLoader_.HasFailedSinceLastBegin();
    }

    void GameWindow::UpdateAsyncSceneLoad()
    {
        sceneLoader_.Step();

        const auto scene = sceneLoader_.TryTakeLoadedScene();
        if (!scene)
            return;

        // AddContent を通さないと FIELD が解決されないので、ChangeMainScene より先に呼ぶ
        AddContent     (scene);
        ChangeMainScene(scene);
        lastAsyncLoadedScene_ = scene;
    }

    void GameWindow::ReleaseUnusedAssetsIfPending()
    {
        // 読み込み中に解放すると、読み込み中のシーンだけが使うアセットまで捨ててしまう
        if (!isAssetReleasePending_ || IsSceneLoading())
            return;

        isAssetReleasePending_ = false;
        std::vector<std::string> sceneFilePaths;
        for (const auto& scene : contents_ | std::views::values)
        {
            sceneFilePaths.push_back(scene->FilePath());
        }
        Module::Asset::AssetPreloader::ReleaseUnused(sceneFilePaths);
    }

    void GameWindow::Play()
    {
        isPlayMode_ = true;
        isPlaying_  = true;
    }

    void GameWindow::Stop()
    {
        isPlayMode_ = false;
    }

    void GameWindow::End()
    {
        isPlayMode_ = false;
        isPlaying_  = false;
        sceneLoader_.Cancel();
        LifeCycle().Coroutine()->AllClear();
        for (const auto& content : contents_ | std::views::values)
        {
            content->RemoveImplementAllGameObject();
        }
        contents_.clear();
        try
        {
            const auto initScene = std::make_shared<Scene::Scene>(Application::Configuration::BuildConfiguration::StartScenePath());
            AddContent(initScene);
            ChangeMainScene(initScene);
        }
        catch (const Module::Exception::NanamiException& exception)
        {
            Module::LogError("GameWindow: 初期シーンの再読み込みに失敗しました: " + std::string(exception.what()));
        }
        Application::ApplicationBase::ResetPhysics();
    }

    std::vector<GameWindow::SceneSnapshot> GameWindow::TakeSceneSnapshots() const
    {
        std::vector<SceneSnapshot> snapshots;
        const auto mainScene = mainScene_.lock();
        for (const auto& scene : contents_ | std::views::values)
        {
            if (!scene)
                continue;
            try
            {
                std::ostringstream stream;
                scene->SaveTo(stream);
                snapshots.push_back({ scene->FilePath(), stream.str(), scene == mainScene });
            }
            catch (const std::exception& exception)
            {
                Module::LogError("GameWindow: シーンの写しを取れませんでした (" + scene->FilePath() + "): " + exception.what());
            }
        }
        return snapshots;
    }

    void GameWindow::UnloadAllScenes()
    {
        isPlayMode_ = false;
        isPlaying_  = false;
        sceneLoader_.Cancel();
        LifeCycle().Coroutine()->AllClear();
        for (const auto& content : contents_ | std::views::values)
        {
            content->RemoveImplementAllGameObject();
        }
        contents_.clear();
        mainScene_.reset();
        lastAsyncLoadedScene_.reset();
        removeGameObjectQueue_ = {};
        Application::ApplicationBase::ResetPhysics();
    }

    void GameWindow::RestoreScenes(const std::vector<SceneSnapshot>& snapshots)
    {
        for (const auto& snapshot : snapshots)
        {
            try
            {
                Scene::Scene::DeserializedContent content;
                std::istringstream stream(snapshot.json);
                Scene::Scene::Deserialize(stream, snapshot.filePath, content, nullptr);
                const auto scene = std::make_shared<Scene::Scene>(snapshot.filePath, std::move(content));
                AddContent(scene);
                if (snapshot.isMain || mainScene_.expired())
                    ChangeMainScene(scene);
            }
            catch (const Module::Exception::NanamiException& exception)
            {
                Module::LogError("GameWindow: シーンを戻せませんでした (" + snapshot.filePath + "): " + std::string(exception.what()));
            }
        }
        if (!contents_.empty())
            return;
        try
        {
            const auto initScene = std::make_shared<Scene::Scene>(Application::Configuration::BuildConfiguration::StartScenePath());
            AddContent(initScene);
            ChangeMainScene(initScene);
        }
        catch (const Module::Exception::NanamiException& exception)
        {
            Module::LogError("GameWindow: 初期シーンの再読み込みに失敗しました: " + std::string(exception.what()));
        }
    }

    std::shared_ptr<Scene::Scene> GameWindow::CatchScene(
        const Guid& guid) const
    {
        if (const auto it = contents_.find(guid); it != contents_.end())
            return it->second;
        
        return nullptr;
    }
    
    void GameWindow::RemoveGameObject(const std::weak_ptr<GameObject::IGameObject>& removeGameObject)
    {
        for (const auto& scene : Scenes())
        {
            if (scene->TryOnRemoveGameObject(removeGameObject))
                break;
        }
        removeGameObjectQueue_.push(removeGameObject);
    }
    
    bool GameWindow::TryReplaceGameObject(
        const Guid& replaceGameObjectGuid,
        const std::shared_ptr<GameObject::IGameObject>& newGameObject) const
    {
        for (const auto& scene : Scenes())
        {
            const auto replaceGameObject = scene->CatchGameObject(replaceGameObjectGuid);
            if (!replaceGameObject)
                continue;
            
            newGameObject->Transform().SetParent(replaceGameObject->Transform().GetParent());
            newGameObject->Transform().SetWorldMatrix(replaceGameObject->Transform().GetWorldMatrix());
            scene->AddGameObject   (newGameObject    );
            scene->RemoveGameObject(replaceGameObject);
            return true;
        }
        return false;
    }
    
    void GameWindow::OnUpdate()
    {
        UpdateAsyncSceneLoad();

        // 非同期ロード中はメインシーンが居ないのが正常。ここで別シーンに差し替えると
        // ResetPhysics と SkipNextFrame(60) がロード 1 回につき二重に走る
        if (!mainScene_.lock() && !IsSceneLoading())
        {
            if (!Scenes().empty())
            {
                ChangeMainScene(Scenes().at(0));
            }
        }
        ReleaseUnusedAssetsIfPending();

        if (isPlayMode_)
        {
            LifeCycle().OnUpdateForGame();
        }
        else
        {
            LifeCycle().OnUpdateForEditor();
            editorCamera_.OnUpdate();
        }
    
        for (const auto& content : contents_ | std::views::values)
        {
            content->OnUpdatePushedContents();
        }
        while (!removeGameObjectQueue_.empty())
        {
            const std::weak_ptr<GameObject::IGameObject>& removeWeak = removeGameObjectQueue_.front();
            if (const std::shared_ptr<GameObject::IGameObject> remove = removeWeak.lock())
            {
                remove->ImplementDestroy();
            }
            removeGameObjectQueue_.pop();
        }
    }
    
    void GameWindow::OnDrawGui(const MainWindowDrawGuiContext context)
    {
        DrawGameObjectMarks();

        ImGui::Begin("GameWindow");
        if (!isPlayMode_)
        {
            ImGui::Text(("LoadingResource Count: " + std::to_string(Asset::Asset::GetLoadingResourceCount())).c_str());
            const auto mainScene = mainScene_.lock();
            ImGui::Text(("currentMainScene: " + (mainScene ? mainScene->Name() : std::string("(none)"))).c_str());
            if (ImGui::Button("Reset EditorCamera"))
            {
                editorCamera_ = Component::Editor3DCamera(); 
            }
            if (ImGui::Button("Play"))
            {
                Play();
            }
            float timeScale = Time::GetTimeScale();
    
            if (ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 20.0f, "%.2f"))
            {
                Time::SetTimeScale(timeScale);
            }
        }
        else
        {
            if (ImGui::Button("Stop"))
            {
                Stop();
            }
            ImGui::SetItemTooltip("Esc + Enter");
            // NOTE: Esc 単体はゲーム内 UI を閉じるのに使うので、停止は Esc + Enter
            const bool isStopChordDown = ImGui::IsKeyDown(ImGuiKey_Escape) && ImGui::IsKeyDown(ImGuiKey_Enter);
            const bool isStopChordPressed = ImGui::IsKeyPressed(ImGuiKey_Escape, false) || ImGui::IsKeyPressed(ImGuiKey_Enter, false);
            if (isStopChordDown && isStopChordPressed)
            {
                Stop();
            }
        }
        if (ImGui::Button("End"))
        {
            End();
        }
        ImGui::End();
    
        ImGui::Begin("Hierarchy");
        
        {
            const bool hasSearchText = hierarchySearchBuffer_[0] != '\0';
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (hasSearchText ? 55.0f : 0.0f));
            ImGui::InputTextWithHint("##HierarchySearch", "Search...", hierarchySearchBuffer_, sizeof(hierarchySearchBuffer_));
            if (hasSearchText)
            {
                ImGui::SameLine();
                if (ImGui::SmallButton("Clear##HierarchySearch"))
                {
                    hierarchySearchBuffer_[0] = '\0';
                }
            }
        }
        const std::string hierarchySearchText = hierarchySearchBuffer_;

        int index = 0;
        std::vector<Scene::Scene*> pendingRemoveScenes;
        for (const auto& content : contents_ | std::views::values)
        {
            const std::string headerLabel = content->Name() + "##" + std::to_string(index);
            if (!hierarchySearchText.empty())
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            if (ImGui::CollapsingHeader(headerLabel.c_str()))
            {
                ImGui::Indent();
                content->OnDrawGui([&pendingRemoveScenes](Scene::Scene* scene) {pendingRemoveScenes.push_back(scene); },
                                context.FileDraggingHand(), hierarchySearchText);
                ImGui::Unindent();
            }
            ++index;
        }
    
        for (Scene::Scene* scene : pendingRemoveScenes)
        {
            auto it = std::ranges::find_if(contents_, [scene](const auto& pair) -> bool {
                return pair.second.get() == scene;
            });
    
            if (it != contents_.end())
            {
                contents_.erase(it);
                isAssetReleasePending_ = true;
            }
        }
        ImGui::End();
    }
    
    void GameWindow::DrawGameObjectMarks() const
    {
        using Application::Configuration::GameWindowConfiguration;

        // ImGui ウィンドウの下、DxLib の 3D 描画の上に重ねる
        ImDrawList& drawList = *ImGui::GetBackgroundDrawList();
        for (const auto& scene : Scenes())
        {
            scene->ForEachGameObject([this, &drawList](const std::shared_ptr<GameObject::IGameObject>& gameObject)
            {
                if (!gameObject || !gameObject->IsEnable())
                    return;

                if (!GameWindowConfiguration::ShouldDrawMark(gameObject->Mark(), isPlayMode_))
                    return;

                const glm::vec3 worldPos  = gameObject->Transform().GetWorldPos();
                const VECTOR    screenPos = ConvWorldPosToScreenPos(VGet(worldPos.x, worldPos.y, worldPos.z));
                // z が 0..1 の外ならカメラの視界外（背後など）
                if (screenPos.z < 0.0f || screenPos.z > 1.0f)
                    return;

                Module::GameObject::DrawMark(drawList, ImVec2(screenPos.x, screenPos.y), gameObject->Mark(), gameObject->Name().c_str());
            });
        }
    }

    void GameWindow::OnSave()
    {
        for (const auto& scene : contents_ | std::views::values)
        {
            scene->OnSave();
        }
    }
}