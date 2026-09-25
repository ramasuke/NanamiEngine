#include "HlslVsFile.h"
#include <DxLib.h>
#include "../../Log/NanamiEngine_Module_Log.h"
#include "../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    HlslVsFile::HlslVsFile(const std::string& contentPath)
        : contentPath_(contentPath)
    {
    }

    HlslVsFile::~HlslVsFile()
    {
        if (vsHandle_ == -1)
            return;

        DeleteShader(vsHandle_);
    }

    void HlslVsFile::OnEnableAsset()
    {
        vsHandle_ = LoadVertexShader(contentPath_.c_str());
        if (vsHandle_ == -1)
            LogError("HlslVsFile: 頂点シェーダーの読み込みに失敗しました: " + contentPath_);
    }

    const Guid& HlslVsFile::GetGuid       () const { return guid_; }
    int         HlslVsFile::GetVsHandle    () const { return vsHandle_; }
    std::string HlslVsFile::GetContentPath () const { return contentPath_; }

    void HlslVsFile::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        LibCore::ImGuiHelper::OnDrawInputField("guid_",        guid_);
        LibCore::ImGuiHelper::OnDrawInputField("vsHandle_",    vsHandle_);
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::HlslVsFile, NanamiEngine::Module::Asset::AssetBase);
NANAMI_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IEnablableAsset, NanamiEngine::Module::Asset::HlslVsFile);
REGISTER_ASSET(HlslVsFile, ".vso")
#pragma endregion
