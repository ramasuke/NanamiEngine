#include "MV1File.h"

namespace NanamiEngine::Module::Asset
{
    Mv1File::Mv1File(const std::string& contentPath)
    {
        contentPath_ = contentPath;
    }
    
    void        Mv1File::OnEnableAsset ()       { dxLibHandle_ = MV1LoadModel(contentPath_.c_str()); }
    const Guid& Mv1File::GetGuid       () const { return guid_; }
    int         Mv1File::DxLibHandle   () const { return MV1DuplicateModel(dxLibHandle_); }
    std::string Mv1File::GetContentPath() const { return contentPath_; }
}
