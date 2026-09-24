#pragma once
#include <atomic>
#include <functional>
#include <string>

namespace NanamiEngine::Module
{
    /** @brief C++例外・SEH例外(nullptr参照、0除算等)から保護して関数を実行する */
    class SafeExecutor final
    {
    public:
        SafeExecutor() = delete;

        // func を C++例外・SEH例外の両方から保護して実行する。
        static bool Execute(const std::function<void()>& func, std::string& outErrorMessage);

        // SEH(nullptr参照等のハードウェア例外)を捕捉して継続するかどうか。
        // NOTE: スキップする(その代わり、壊れた可能性のある状態のまま処理を続ける前提を受け入れることになる)。
        [[nodiscard]] static bool IsCrashRecoveryEnabled();
        static void SetCrashRecoveryEnabled(bool enabled);

        // true(デフォルト)の場合、デバッガ(Rider/Visual Studio等)がアタッチされている間は、
        // IsCrashRecoveryEnabled() が true でもそれを無視し、SEHを常に素通りさせて通常通り
        // クラッシュ(デバッガがその場で停止)させる。デバッグ中でもコンポーネント単位の継続動作
        // 自体を確認したい場合はfalseにする。
        [[nodiscard]] static bool IsDebuggerFailFastEnabled();
        static void SetDebuggerFailFastEnabled(bool enabled);

    private:
        // NOTE: SEH フィルタ式から読むので atomic
        static std::atomic<bool> crashRecoveryEnabled_;
        static std::atomic<bool> debuggerFailFastEnabled_;
    };
}
