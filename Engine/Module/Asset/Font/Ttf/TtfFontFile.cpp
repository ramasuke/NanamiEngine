#include "TtfFontFile.h"
#include "../../../../../Libs/LibCore/DxLib/ShiftJis.h"

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

    TtfFontFile::~TtfFontFile()
    {
        if (dxLibHandle_ != -1)
            DeleteFontToHandle(dxLibHandle_);
        if (!addedFontResourcePath_.empty())
            RemoveFontResourceExA(addedFontResourcePath_.c_str(), FR_PRIVATE, nullptr);
    }

    void TtfFontFile::OnEnableAsset()
    {
        if (AddFontResourceExA(contentPath_.c_str(), FR_PRIVATE, nullptr) > 0)
            addedFontResourcePath_ = contentPath_;
        dxLibHandle_ = CreateFontToHandle(LibCore::Dxlib::Utf8ToShiftJis(fontName_).c_str(), size_, thickness_, fontType_);
    }
}
