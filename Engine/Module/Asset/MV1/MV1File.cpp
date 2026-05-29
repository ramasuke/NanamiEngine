#include "MV1File.h"
#include "DxLib.h"

namespace NanamiEngine::Module::Asset
{
    Mv1File::Mv1File(const std::string& contentPath)
        : contentPath_(contentPath)
    {
    }
    
    void Mv1File::OnEnableAsset() { dxLibHandle_ = MV1LoadModel(contentPath_.c_str()); }

    void Mv1File::OnDrawGui()
    {
        LibCore::ImGuiHelper::OnDrawInputField("contentPath_", contentPath_);
        LibCore::ImGuiHelper::OnDrawInputField("guid_", guid_);
        LibCore::ImGuiHelper::OnDrawInputField("dxLibHandle_", dxLibHandle_);
    }

    const Guid& Mv1File::GetGuid        () const { return guid_; }
    int         Mv1File::LoadDxLibHandle() const { return MV1DuplicateModel(dxLibHandle_); }
    std::string Mv1File::GetContentPath () const { return contentPath_; }
}
