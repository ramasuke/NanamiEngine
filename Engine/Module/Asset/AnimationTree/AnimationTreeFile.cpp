#include "AnimationTreeFile.h"

#include <utility>

#include "../../../Core/Application/Window/Main/Animator/AnimatorWindow.h"
#include "../../Exception/Engine_Module_Exception.h"
#include "../../Log/NanamiEngine_Module_Log.h"

Asset::AnimationTreeFile::AnimationTreeFile(std::string contentPath)
    : contentPath_(std::move(contentPath))
{

}

std::shared_ptr<AnimationTree::AnimationTree> Asset::AnimationTreeFile::OnLoadCopyContent() const
{
    try
    {
        return std::make_shared<AnimationTree::AnimationTree>(contentPath_);
    }
    catch (const NanamiEngine::Module::Exception::SerializationException& exception)
    {
        // 壊れた AnimationTree は「無い」ものとして扱う。Animator 側は animationTree_ の null チェックで動作を続ける
        NanamiEngine::Module::LogError("AnimationTreeFile: " + std::string(exception.what()));
        return nullptr;
    }
}

std::string Asset::AnimationTreeFile::GetContentPath() const
{
    return contentPath_;
}

void Asset::AnimationTreeFile::OnDoubleClick()
{
    const auto animationTree = OnLoadCopyContent();
    if (!animationTree)
        return;

    Core::Application::ApplicationBase::OnChangeWindow(Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::AnimatorWindow>());
    Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::AnimatorWindow>()->AddContent(animationTree);
}

void Asset::AnimationTreeFile::OnSaveCallback()
{
    std::make_shared<AnimationTree::AnimationTree>(contentPath_)->OnSave();
}

void Asset::AnimationTreeFile::OnDrawGui()
{
    LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
    LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
}
