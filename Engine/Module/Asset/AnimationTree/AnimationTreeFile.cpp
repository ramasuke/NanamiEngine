#include "AnimationTreeFile.h"

#include <utility>

#include "../../../Core/Application/Window/Main/Animator/AnimatorWindow.h"

Asset::AnimationTreeFile::AnimationTreeFile(std::string contentPath)
    : contentPath_(std::move(contentPath))
{
    
}

std::string Asset::AnimationTreeFile::GetContentPath() const
{
    return contentPath_;
}

void Asset::AnimationTreeFile::OnDoubleClick()
{
    Core::Application::ApplicationBase::OnChangeWindow(Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::AnimatorWindow>());
    Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::AnimatorWindow>()->AddContent(OnLoadCopyContent());
}

void Asset::AnimationTreeFile::OnSaveCallback()
{
    std::make_shared<AnimationTree::AnimationTree>(contentPath_)->OnSave();
}
