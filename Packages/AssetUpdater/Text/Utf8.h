#pragma once
#include <string>

namespace NanamiEngine::AssetUpdater
{
    /** 不正な UTF-8 なら空文字を返す */
    [[nodiscard]] std::wstring Utf8ToWide(const std::string& utf8);
    [[nodiscard]] std::string  WideToUtf8(const std::wstring& wide);
}
