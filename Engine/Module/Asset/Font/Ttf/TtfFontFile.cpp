#include "TtfFontFile.h"
#include <algorithm>
#include <cmath>
#include "../../../../../Libs/LibCore/DxLib/ShiftJis.h"
#include "../../../Serialization/Engine_Module_SerializationRegistration.h"

namespace NanamiEngine::Module::Asset
{
    TtfFontFile::TtfFontFile(std::string contentPath)
        : contentPath_(std::move(contentPath)  )
        , fontName_   ("fontName"              )
        , size_       (20                      )
        , thickness_  (3                       )
        , fontType_   (DX_FONTTYPE_ANTIALIASING)
        , edgeSize_   (-1                      )
        , edgeColor_  (0, 0, 0                 )
    {

    }

    TtfFontFile::~TtfFontFile()
    {
        for (const auto& [pixelSize, handle] : sizedHandles_)
            DeleteFontToHandle(handle);
        if (dxLibHandle_ != -1)
            DeleteFontToHandle(dxLibHandle_);
        if (!addedFontResourcePath_.empty())
            RemoveFontResourceExA(addedFontResourcePath_.c_str(), FR_PRIVATE, nullptr);
    }

    void TtfFontFile::OnEnableAsset()
    {
        if (AddFontResourceExA(contentPath_.c_str(), FR_PRIVATE, nullptr) > 0)
            addedFontResourcePath_ = contentPath_;
        dxLibHandle_ = CreateFontToHandle(LibCore::Dxlib::Utf8ToShiftJis(fontName_).c_str(), size_, thickness_, fontType_, -1, edgeSize_);
    }

    int TtfFontFile::HandleForPixelSize(const int pixelSize)
    {
        if (dxLibHandle_ == -1 || pixelSize == size_ || pixelSize <= 0)
            return dxLibHandle_;

        if (const auto found = sizedHandles_.find(pixelSize); found != sizedHandles_.end())
            return found->second;
        
        const int edgeSize = edgeSize_ > 0
            ? (std::max)(1, static_cast<int>(std::lround(static_cast<float>(edgeSize_) * static_cast<float>(pixelSize) / static_cast<float>(size_))))
            : edgeSize_;
        
        const int useASyncLoad = GetUseASyncLoadFlag();
        SetUseASyncLoadFlag(FALSE);
        const int handle = CreateFontToHandle(LibCore::Dxlib::Utf8ToShiftJis(fontName_).c_str(), pixelSize, thickness_, fontType_, -1, edgeSize);
        SetUseASyncLoadFlag(useASyncLoad);

        if (handle == -1)
            return dxLibHandle_;

        sizedHandles_.emplace(pixelSize, handle);
        return handle;
    }
}

#pragma region SerializationMacro
NANAMI_REGISTER_TYPE(NanamiEngine::Module::Asset::TtfFontFile, NanamiEngine::Module::Asset::AssetBase);
NANAMI_REGISTER_POLYMORPHIC_RELATION(NanamiEngine::Module::LifeCycleCallback::IEnablableAsset, NanamiEngine::Module::Asset::TtfFontFile);
REGISTER_ASSET(TtfFontFile, ".ttf")
#pragma endregion
