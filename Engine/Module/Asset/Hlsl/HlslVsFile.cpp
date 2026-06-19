#include "HlslVsFile.h"
#include <DxLib.h>

namespace NanamiEngine::Module::Asset
{
    HlslVsFile::HlslVsFile(const std::string& contentPath)
        : contentPath_(contentPath)
    {
    }

    void HlslVsFile::OnEnableAsset()
    {
        vsHandle_ = LoadVertexShader(contentPath_.c_str());
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
