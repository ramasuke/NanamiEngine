#pragma once
#include "Engine/Core/Api/NanamiApi.h"
#include <atomic>
#include <functional>
#include <string>

namespace NanamiEngine::Module
{
    // NOTE: C++例外・SEH例外(nullptr参照、0除算等)から保護して関数を実行する
    class NANAMI_API SafeExecutor final
    {
    public:
        SafeExecutor() = delete;

        // NOTE: 例外や SEH で落ちたら false を返し、outErrorMessage に内容を入れる
        static bool Execute(const std::function<void()>& func, std::string& outErrorMessage);

        // NOTE: SEH(nullptr参照等)を捕捉して、壊れたかもしれない状態のまま処理を続けるか
        [[nodiscard]] static bool IsCrashRecoveryEnabled();
        static void SetCrashRecoveryEnabled(bool enabled);

        // NOTE: true(既定)ならデバッガのアタッチ中は SEH を捕捉せず、その場でクラッシュさせる
        [[nodiscard]] static bool IsDebuggerFailFastEnabled();
        static void SetDebuggerFailFastEnabled(bool enabled);

    private:
        // NOTE: SEH フィルタ式から読むので atomic
        static std::atomic<bool> crashRecoveryEnabled_;
        static std::atomic<bool> debuggerFailFastEnabled_;
    };
}
