#include "GameWindow.h"

#include <memory>
#include <ranges>

#include "ImGuiHelper.h"
#include "../../../../../Module/Asset/Asset.h"
#include "../../../../../Module/GameObject/Transform/Transform.h"
#include "../../../../Coroutine/Scheduler/CoroutineScheduler.h"
#include "../../../Time/Time.h"

namespace NanamiEngine::Core::MainWindow
{
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
    
    void GameWindow::ChangeMainScene(const std::shared_ptr<Scene::Scene>& scene)
    {
        mainScene_ = scene;
    }
    
    std::shared_ptr<Scene::Scene> GameWindow::CatchScene(
        const Guid& guid) const
    {
        if (const auto it = contents_.find(guid); it != contents_.end())
            return it->second;
        
        return nullptr;
    }
    
    void GameWindow::RemoveGameObject(const std::weak_ptr<GameObject::IGameObject>& removeGameObject) const
    {
        for (const auto& scene : Scenes())
        {
            if (scene->TryOnRemoveGameObject(removeGameObject))
                break;
        }
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
            
            newGameObject->TransformRef().SetParent(replaceGameObject->TransformRef().GetParent());
            newGameObject->TransformRef().SetWorldMatrix(replaceGameObject->TransformRef().GetWorldMatrix());
            scene->AddGameObject   (newGameObject    );
            scene->RemoveGameObject(replaceGameObject);
            return true;
        }
        return false;
    }
    
    void GameWindow::OnUpdate()
    {
        if (!mainScene_.lock())
        {
            if (!Scenes().empty())
            {
                ChangeMainScene(Scenes().at(0));
            }
        }
        
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
    }
    
    void GameWindow::OnDrawGui(const MainWindowDrawGuiContext context)
    {
        ImGui::Begin("GameWindow");
        if (!isPlayMode_)
        {
            ImGui::Text(("LoadingResource Count: " + std::to_string(Asset::Asset::GetLoadingResourceCount())).c_str());
            ImGui::Text(("currentMainScene: " + mainScene_.lock()->Name()).c_str());
            if (ImGui::Button("Play"))
            {
                isPlayMode_ = true;
                isPlaying_  = true;
            }
            float timeScale = Time::GetTimeScale();
    
            if (ImGui::SliderFloat("Time Scale", &timeScale, 0.0f, 10.0f, "%.2f"))
            {
                Time::SetTimeScale(timeScale);
            }
        }
        else
        {
            // Stopボタン
            if (ImGui::Button("Stop"))
            {
                isPlayMode_ = !isPlayMode_;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                isPlayMode_ = !isPlayMode_;
            }
        }
        if (ImGui::Button("End"))
        {
            isPlayMode_ = false;
            isPlaying_  = false;
            LifeCycle().Coroutine()->AllClear();
            for (const auto& content : contents_ | std::views::values)
            {
                content->RemoveImplementAllGameObject();
            }
            contents_.clear();
            const auto initScene = std::make_shared<Scene::Scene>("Assets/Scene/GameManage.scene");
            AddContent(initScene);
            ChangeMainScene(initScene); 
        }
        ImGui::End();
    
        ImGui::Begin("Hierarchy");
        int index = 0;
        std::vector<Scene::Scene*> pendingRemoveScenes;
        for (const auto& content : contents_ | std::views::values)
        {
            const std::string headerLabel = content->Name() + "##" + std::to_string(index);
            if (ImGui::CollapsingHeader(headerLabel.c_str()))
            {
                ImGui::Indent();
                content->OnDrawGui([&pendingRemoveScenes](Scene::Scene* scene) {pendingRemoveScenes.push_back(scene); },
                                context.FileDraggingHand());
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
            }
        }
        ImGui::End();
    }
    
    void GameWindow::OnSave()
    {
        for (const auto& scene : contents_ | std::views::values)
        {
            scene->OnSave();
        }
    }
}