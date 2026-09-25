#pragma once
#include "NanamiApi.h"

// ゲーム DLL を差し替えるとき、その DLL の登録だけをレジストリから消すのに使う。
namespace NanamiEngine::Core
{
    /** @brief address が置かれているモジュールのハンドル (HMODULE)。分からなければ nullptr */
    [[nodiscard]] NANAMI_API void* ModuleOf(const void* address);

    /** @brief object の vtable が置かれているモジュール。多相型のインスタンスがどの DLL のクラスかを知る */
    [[nodiscard]] NANAMI_API void* ModuleOfVTable(const void* object);
}

/** @brief この式を書いた翻訳単位が属するモジュール。登録マクロやテンプレートの中で使う */
#define NANAMI_CURRENT_MODULE() ([]() -> void* { static const int nanamiModuleAnchor = 0; return ::NanamiEngine::Core::ModuleOf(&nanamiModuleAnchor); }())
