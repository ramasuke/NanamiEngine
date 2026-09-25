#include "HlslPsFile.h"
#include <DxLib.h>
#include "../../Log/NanamiEngine_Module_Log.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    HlslPsFile::HlslPsFile(const std::string& contentPath)
        : contentPath_(contentPath)
    {
    }

    HlslPsFile::~HlslPsFile()
    {
        if (psHandle_ == -1)
            return;

        DeleteShader(psHandle_);
    }

    void HlslPsFile::OnEnableAsset()
    {
        psHandle_ = LoadPixelShader(contentPath_.c_str());
        if (psHandle_ == -1)
            LogError("HlslPsFile: ピクセルシェーダーの読み込みに失敗しました: " + contentPath_);
    }

    const Guid& HlslPsFile::GetGuid       () const { return guid_; }
    int         HlslPsFile::GetPsHandle    () const { return psHandle_; }
    std::string HlslPsFile::GetContentPath () const { return contentPath_; }

    void HlslPsFile::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        LibCore::ImGuiHelper::OnDrawInputField("guid_",        guid_);
        LibCore::ImGuiHelper::OnDrawInputField("psHandle_",    psHandle_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::HlslPsFile, NanamiEngine::Module::Asset::AssetBase);
NANAMI_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IEnablableAsset, NanamiEngine::Module::Asset::HlslPsFile);
REGISTER_ASSET(HlslPsFile, ".pso")
#pragma endregion
