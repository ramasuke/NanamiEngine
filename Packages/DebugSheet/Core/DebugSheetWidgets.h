#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <initializer_list>
#include <string_view>

/**
 * ページ実装で使う部品。全部シートのデザイン (全幅のリストセル) で描く
 */
namespace NanamiEngine::DebugSheet::Widgets
{
    /** @brief 区切り見出し */
    NANAMI_API void Header(std::string_view text);
    /** @brief 補足の小さい文字 */
    NANAMI_API void Note(std::string_view text);
    /** @brief 全幅のセル。押されたら true */
    NANAMI_API bool Button(std::string_view label);
    /** @brief 右端に > が付いたセル。子ページへ進むときに使う */
    NANAMI_API bool NavigationCell(std::string_view label);
    /** @brief 右端にスイッチが付いたセル。切り替わったら true */
    NANAMI_API bool Toggle(std::string_view label, bool& value);
    /** @brief 左に項目名、右に値 */
    NANAMI_API void Label(std::string_view key, std::string_view value);
    /**
     * @brief 左に項目名、右に小さいボタンを並べた行
     * @return 押されたボタンの番号。押されていなければ -1
     */
    NANAMI_API int ButtonRow(std::string_view label, std::initializer_list<const char*> buttons);
    /**
     * @brief 2 回押しで実行するセル。1 回目で確認表示に変わり、数秒で戻る
     * @param id ページ内で一意な名前
     */
    NANAMI_API bool ConfirmButton(std::string_view label, std::string_view id);
    /** @brief 全幅の整数入力 */
    NANAMI_API bool InputInt(std::string_view label, int& value, int step = 1);
}
