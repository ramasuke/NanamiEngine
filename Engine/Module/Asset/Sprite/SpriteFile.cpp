#include "SpriteFile.h"
#include "DxLib.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    SpriteFile::SpriteFile(std::string contentPath)
        : contentPath_(std::move(contentPath))
    {
    }

    SpriteFile::~SpriteFile()
    {
        if (dxLibId_ == -1)
            return;

        DeleteGraph(dxLibId_);
    }

    void SpriteFile::OnEnableAsset()
    {
    }

    int SpriteFile::GetDxLibHandle() const
    {
        if (!isLoadAttempted_)
        {
            const int useASyncLoad = GetUseASyncLoadFlag();
            SetUseASyncLoadFlag(FALSE);
            RequestLoad();
            SetUseASyncLoadFlag(useASyncLoad);
        }
        return dxLibId_;
    }

    void SpriteFile::RequestLoad() const
    {
        // 読み込みに失敗したファイルを毎フレーム読み直さないよう、Unload されるまでは 1 回だけ試す
        if (isLoadAttempted_)
            return;

        isLoadAttempted_ = true;
        dxLibId_ = LoadGraph();
    }

    void SpriteFile::Unload()
    {
        if (dxLibId_ != -1)
            DeleteGraph(dxLibId_);

        dxLibId_         = -1;
        isLoadAttempted_ = false;
    }

    int SpriteFile::LoadGraph() const
    {
        return DxLib::LoadGraph(contentPath_.c_str());
    }

    void SpriteFile::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
        ImGui::Text(("dxLibId: " + std::to_string(dxLibId_)).c_str());
    }

    std::string SpriteFile::GetContentPath() const
    {
        return contentPath_;
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::SpriteFile, NanamiEngine::Module::Asset::AssetBase);
NANAMI_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IEnablableAsset, NanamiEngine::Module::Asset::SpriteFile);
REGISTER_ASSET(SpriteFile, ".png")
#pragma endregion
