#include "SceneFile.h"

#include <utility>

#include "../../../Core/Application/Window/Main/Game/GameWindow.h"

Asset::SceneFile::SceneFile(std::string contentPath) : contentPath_(std::move(contentPath))
{
}

std::shared_ptr<Scene::Scene> Asset::SceneFile::LoadScene() const
{
    return std::make_shared<Scene::Scene>(contentPath_);
}

std::string Asset::SceneFile::GetContentPath() const
{
    return contentPath_;
}

void Asset::SceneFile::OnDoubleClick()
{
    Core::Application::ApplicationBase::OnChangeWindow<Core::MainWindow::GameWindow>();
    Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::GameWindow>()->AddContent(LoadScene());
}   
