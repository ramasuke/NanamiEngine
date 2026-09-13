#pragma once
#include <string>
#include "DxLib.h"

namespace LibCore::Dxlib
{
    // MultiByte ビルドの DxLib は文字列引数を Shift-JIS として解釈する
    inline std::string Utf8ToShiftJis(const std::string& utf8)
    {
        int wideSize = MultiByteToWideChar(
            CP_UTF8, 0,
            utf8.c_str(), -1,
            nullptr, 0
        );

        std::wstring wide(wideSize, L'\0');
        MultiByteToWideChar(
            CP_UTF8, 0,
            utf8.c_str(), -1,
            wide.data(), wideSize
        );

        int sjisSize = WideCharToMultiByte(
            932, 0,
            wide.c_str(), -1,
            nullptr, 0,
            nullptr, nullptr
        );

        std::string sjis(sjisSize, '\0');
        WideCharToMultiByte(
            932, 0,
            wide.c_str(), -1,
            sjis.data(), sjisSize,
            nullptr, nullptr
        );

        return sjis;
    }
}
