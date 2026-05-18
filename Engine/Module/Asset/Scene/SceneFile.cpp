#include "SceneFile.h"

#include <utility>

#include "../../../Core/Application/Window/Main/Game/GameWindow.h"

namespace NanamiEngine::Module::Asset
{
    SceneFile::SceneFile(std::string contentPath)
    : contentPath_(std::move(contentPath))
    {
    }
    
    std::shared_ptr<Scene::Scene> SceneFile::LoadScene() const
    {
        return std::make_shared<Scene::Scene>(contentPath_);
    }
    
    std::string SceneFile::GetContentPath() const
    {
        return contentPath_;
    }
    
    void SceneFile::OnDoubleClick()
    {
        Core::Application::ApplicationBase::OnChangeWindow<Core::MainWindow::GameWindow>();
        Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::GameWindow>()->AddContent(LoadScene());
    }
    
    void SceneFile::CopiedInit()
    {
        guid_ = Guid();
        auto scene = LoadScene();
        scene->CopiedInit();
        scene->OnSave();
    }
    
    void SceneFile::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        ImGuiHelper::OnDrawInputField("guid_", guid_);
    }   
}
