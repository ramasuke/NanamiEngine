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

    // DxLib が返す文字列(モデルのフレーム名・アニメ名など)を ImGui 表示用に UTF-8 へ戻す
    inline std::string ShiftJisToUtf8(const std::string& sjis)
    {
        const int wideSize = MultiByteToWideChar(932, 0, sjis.c_str(), -1, nullptr, 0);
        if (wideSize <= 0)
            return {};

        std::wstring wide(wideSize, L'\0');
        MultiByteToWideChar(932, 0, sjis.c_str(), -1, wide.data(), wideSize);

        const int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (utf8Size <= 0)
            return {};

        std::string utf8(utf8Size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, utf8.data(), utf8Size, nullptr, nullptr);
        utf8.resize(utf8Size - 1);
        return utf8;
    }
}
