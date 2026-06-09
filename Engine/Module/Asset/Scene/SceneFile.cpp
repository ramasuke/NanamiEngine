#include "SceneFile.h"

#include <utility>
#include <filesystem>

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
        Core::Application::ApplicationBase::GameWindow()->AddContent(LoadScene());
    }

    void SceneFile::CopiedInit()
    {
        guid_ = Guid();
        auto scene = LoadScene();

        const std::filesystem::path path(contentPath_);
        const auto stem      = path.stem().string();        // "scene"
        const auto extension = path.extension().string();   // ".nscene"
        const auto parent    = path.parent_path().string(); // "path/to"
        const std::string newName = stem + "_copy" + extension;

        std::filesystem::path newPath;
        if (!parent.empty())
            newPath = parent + "/" + newName;
        else
            newPath = newName;

        scene->CopiedInit(newPath.string());
        scene->OnSave();
        contentPath_ = newPath.string();
    }
    
    void SceneFile::OnDrawGui()
    {
        ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        ImGuiHelper::OnDrawInputField("guid_", guid_);
    }   
}
