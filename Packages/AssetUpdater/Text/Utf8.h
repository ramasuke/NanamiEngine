#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <string>

namespace NanamiEngine::AssetUpdater
{
    [[nodiscard]] NANAMI_API std::wstring Utf8ToWide(const std::string& utf8);
    [[nodiscard]] NANAMI_API std::string  WideToUtf8(const std::wstring& wide);
}
