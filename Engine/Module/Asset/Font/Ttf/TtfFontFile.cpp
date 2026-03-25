#include "TtfFontFile.h"

namespace NanamiEngine::Module::Asset
{
    TtfFontFile::TtfFontFile(std::string contentPath)
        : contentPath_(std::move(contentPath)  )
        , fontName_   ("fontName"              )
        , size_       (20                      )
        , thickness_  (3                       )
        , fontType_   (DX_FONTTYPE_ANTIALIASING)
    {
        
    }
    
    void TtfFontFile::OnEnableAsset()
    {
        AddFontResourceExA(contentPath_.c_str(), FR_PRIVATE, nullptr);
        dxLibHandle_ = CreateFontToHandle(fontName_.c_str(), size_, thickness_, fontType_);
    }
}