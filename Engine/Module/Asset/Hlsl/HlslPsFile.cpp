#include "HlslPsFile.h"
#include <DxLib.h>

namespace NanamiEngine::Module::Asset
{
    HlslPsFile::HlslPsFile(const std::string& contentPath)
        : contentPath_(contentPath)
    {
    }

    void HlslPsFile::OnEnableAsset()
    {
        psHandle_ = LoadPixelShader(contentPath_.c_str());
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
