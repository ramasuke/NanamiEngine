#pragma once
#include "NanamiApi.h"

namespace NanamiEngine::Core
{
    // NOTE: exe / DLL を区別する ID
    class NANAMI_API ModuleHandle
    {
    public:
        constexpr ModuleHandle() = default;
        constexpr explicit ModuleHandle(void* raw) : raw_(raw) {}

        [[nodiscard]] constexpr void* Raw    () const { return raw_; }
        [[nodiscard]] constexpr bool  IsValid() const { return raw_ != nullptr; }
        friend constexpr bool operator==(ModuleHandle, ModuleHandle) = default;

    private:
        void* raw_ = nullptr;
    };

    // NOTE: address が置かれているモジュール
    [[nodiscard]] NANAMI_API ModuleHandle ModuleOf(const void* address);

    // NOTE: 多相型のインスタンスがどの DLL のクラスかを知る
    [[nodiscard]] NANAMI_API ModuleHandle ModuleOfVTable(const void* object);
}

// NOTE: この式を書いた翻訳単位が属するモジュール。登録マクロやテンプレートの中で使う
#define NANAMI_CURRENT_MODULE() ([]() -> ::NanamiEngine::Core::ModuleHandle { static const int nanamiModuleAnchor = 0; return ::NanamiEngine::Core::ModuleOf(&nanamiModuleAnchor); }())
