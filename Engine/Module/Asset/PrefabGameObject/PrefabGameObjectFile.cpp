#include "PrefabGameObjectFile.h"

#include <filesystem>

#include "../../../Core/Application/Window/Main/PrefabView/PrefabViewWindow.h"
#include "../../../Core/Network/Object/PrefabRegistry/NetworkPrefabObjectRegistry.h"
#include "../../Network/Object/Component/GameObject/Engine_Network_NetworkGameObject.h"
#include "../../Exception/Engine_Module_Exception.h"
#include "../../Log/NanamiEngine_Module_Log.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    PrefabGameObjectFile::PrefabGameObjectFile(std::string contentPath)
        : contentPath_(std::move(contentPath))
    {
    }
    
    void PrefabGameObjectFile::OnEnableAsset()
    {
        try
        {
            content_ = std::make_shared<GameObject::PrefabGameObject>(contentPath_);
        }
        catch (const Exception::SerializationException& exception)
        {
            NanamiEngine::Module::LogError("PrefabGameObjectFile: " + std::string(exception.what()));
            return;
        }
    
        //NetworkObjectの場合
        if (content_->Components().Catch<Network::NetworkGameObject>().lock())
        {
            Core::Application::ApplicationBase::NetworkPrefabObjectRegistry().Add(content_);
        }
    }
    
    std::string PrefabGameObjectFile::GetContentPath() const
    {
        return contentPath_;
    }
    
    void PrefabGameObjectFile::OnDoubleClick()
    {
        if (!content_)
        {
            NanamiEngine::Module::LogError("PrefabGameObjectFile: 読み込みに失敗しているため開けません: " + contentPath_);
            return;
        }
    
        const auto prefabWindow = Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::PrefabViewWindow>();
        Core::Application::ApplicationBase::OnChangeWindow(prefabWindow);
    
        if (prefabWindow->Contains(content_->GetGuid()))
            return;
    
        const auto workingCopy = content_->CreateWorkingCopy();
        prefabWindow->AddContent(workingCopy);
        workingCopy->InitGameObject(std::weak_ptr<GameObject::IGameObject>(), workingCopy);
        workingCopy->InitPrefab(contentPath_);
    }
    
    void Asset::PrefabGameObjectFile::OnSaveCallback()
    {
        if (!content_)
            return;
    
        const auto prefabWindow = Core::Application::ApplicationBase::MainWindows().Catch<Core::MainWindow::PrefabViewWindow>();
        if (prefabWindow && prefabWindow->Contains(content_->GetGuid()))
        {
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
    
    void PrefabGameObjectFile::OnRenamed(const std::string& newContentPath)
    {
        contentPath_ = newContentPath;
        if (content_)
            content_->InitPrefab(newContentPath);
    }
    
    void PrefabGameObjectFile::CopiedInit()
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
    
    void PrefabGameObjectFile::OnDrawGui()
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
}

#pragma region SerializationMacro
CEREAL_REGISTER_TYPE(NanamiEngine::Module::Asset::PrefabGameObjectFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::Asset::AssetBase, NanamiEngine::Module::Asset::PrefabGameObjectFile);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IEnablableAsset, NanamiEngine::Module::Asset::PrefabGameObjectFile);
REGISTER_ASSET(PrefabGameObjectFile, PREFAB_FILE_EXTENSION_LABEL)
REGISTER_CREATABLE_ASSET_EXTENSION("Prefab", PREFAB_FILE_EXTENSION_LABEL, "Scene")
#pragma endregion
