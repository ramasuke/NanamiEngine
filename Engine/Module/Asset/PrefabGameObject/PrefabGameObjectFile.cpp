#include "PrefabGameObjectFile.h"

#include "../../../Core/Application/Window/Main/PrefabView/PrefabViewWindow.h"

Asset::PrefabGameObjectFile::PrefabGameObjectFile(std::string contentPath)
    : contentPath_(std::move(contentPath))
{
}

void Asset::PrefabGameObjectFile::OnEnableAsset()
{
    content_ = std::make_shared<GameObject::PrefabGameObject>(contentPath_);
}

std::string Asset::PrefabGameObjectFile::GetContentPath() const
{
    return contentPath_;
}

void Asset::PrefabGameObjectFile::OnDoubleClick()
{
    Core::Application::ApplicationBase::OnChangeWindow(Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::PrefabViewWindow>());
    Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::PrefabViewWindow>()->AddContent(content_);
    content_->InitGameObject(std::weak_ptr<GameObject::IGameObject>(), content_);
    content_->InitPrefab(contentPath_);
}

void Asset::PrefabGameObjectFile::OnSaveCallback()
{
    content_->OnSave();
}

void Asset::PrefabGameObjectFile::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
    ImGuiHelper::OnDrawInputField("guid_", guid_);

    if (ImGui::TreeNode(("Content##" + guid_.Value()).c_str()))
    {
        content_->OnDrawGui();
        ImGui::TreePop();
        ImGui::Spacing();
    }
    
    if (ImGui::Button(("ReplaceSceneObjects##" + guid_.Value()).c_str()))
    {
        content_->OnReplaceCopiedObjects();
    }
}
