#include "PrefabGameObjectFile.h"

#include <filesystem>

#include "../../../Core/Application/Window/Main/PrefabView/PrefabViewWindow.h"
#include "../../../Core/Network/Object/PrefabRegistry/NetworkPrefabObjectRegistry.h"
#include "../../Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../Exception/Engine_Module_Exception.h"
#include "../../Log/NanamiEngine_Module_Log.h"

Asset::PrefabGameObjectFile::PrefabGameObjectFile(std::string contentPath)
    : contentPath_(std::move(contentPath))
{
}

void Asset::PrefabGameObjectFile::OnEnableAsset()
{
    try
    {
        content_ = std::make_shared<GameObject::PrefabGameObject>(contentPath_);
    }
    catch (const NanamiEngine::Module::Exception::SerializationException& exception)
    {
        // 壊れた Prefab は content_ を null のままにする。利用側（Instantiate / ダブルクリック / Save）は null チェックで継続する
        NanamiEngine::Module::LogError("PrefabGameObjectFile: " + std::string(exception.what()));
        return;
    }

    //NetworkObjectの場合
    if (content_->Components().Catch<Network::NetworkGameObject>().lock())
    {
        Core::Application::ApplicationBase::NetworkPrefabObjectRegistry().Add(content_);
    }
}

std::string Asset::PrefabGameObjectFile::GetContentPath() const
{
    return contentPath_;
}

void Asset::PrefabGameObjectFile::OnDoubleClick()
{
    if (!content_)
        return;

    const auto prefabWindow = Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::PrefabViewWindow>();
    Core::Application::ApplicationBase::OnChangeWindow(prefabWindow);

    if (prefabWindow->Contains(content_->GetGuid()))
        return; // 既に開いている場合は編集中の内容を保持し、作り直さない

    const auto workingCopy = content_->CreateWorkingCopy();
    prefabWindow->AddContent(workingCopy);
    workingCopy->InitGameObject(std::weak_ptr<GameObject::IGameObject>(), workingCopy);
    workingCopy->InitPrefab(contentPath_);
}

void Asset::PrefabGameObjectFile::OnSaveCallback()
{
    // 読み込みに失敗した Prefab は空データで上書きしない
    if (!content_)
        return;

    const auto prefabWindow = Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::PrefabViewWindow>();
    if (prefabWindow && prefabWindow->Contains(content_->GetGuid()))
    {
        // Prefab ウィンドウの作業用コピーが直前の MainWindows().OnSave() で
        // 既にファイルへ正しい内容を書き込み済み。content_ を古いメモリ内容のまま
        // 再保存して上書きしてしまわないよう、ディスクの最新状態から作り直す。
        try
        {
            content_ = std::make_shared<GameObject::PrefabGameObject>(contentPath_);
        }
        catch (const NanamiEngine::Module::Exception::SerializationException& exception)
        {
            NanamiEngine::Module::LogError("PrefabGameObjectFile: " + std::string(exception.what()));
            return;
        }

        if (content_->Components().Catch<Network::NetworkGameObject>().lock())
        {
            Core::Application::ApplicationBase::NetworkPrefabObjectRegistry().Add(content_);
        }
        return;
    }

    content_->OnSave();
}

void Asset::PrefabGameObjectFile::OnRenamed(const std::string& newContentPath)
{
    contentPath_ = newContentPath;
    if (content_)
        content_->InitPrefab(newContentPath);
}

void Asset::PrefabGameObjectFile::CopiedInit()
{
    guid_ = Guid();

    const auto content = std::make_shared<GameObject::PrefabGameObject>(contentPath_);

    const std::filesystem::path path(contentPath_);
    const auto stem      = path.stem().string();
    const auto extension = path.extension().string();
    const auto parent    = path.parent_path().string();
    const std::string newName = stem + "_copy" + extension;

    std::filesystem::path newPath;
    if (!parent.empty())
        newPath = parent + "/" + newName;
    else
        newPath = newName;

    content->CopiedInit(newPath.string());
    content->OnSave();
    contentPath_ = newPath.string();
}

void Asset::PrefabGameObjectFile::OnDrawGui()
{
    ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
    ImGuiHelper::OnDrawInputField("guid_", guid_);

    if (!content_)
        return;
    
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
