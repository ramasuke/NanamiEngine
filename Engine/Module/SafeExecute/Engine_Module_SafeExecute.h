#pragma once
#include <functional>
#include <string>

namespace NanamiEngine::Module
{
    // SEH(nullptr参照等のハードウェア例外)を捕捉して継続するかどうかのグローバル設定。
    // NOTE: スキップする(その代わり、壊れた可能性のある状態のまま処理を続ける前提を受け入れることになる)。
    bool IsCrashRecoveryEnabled();
    void SetCrashRecoveryEnabled(bool enabled);

    // true(デフォルト)の場合、デバッガ(Rider/Visual Studio等)がアタッチされている間は、
    // IsCrashRecoveryEnabled() が true でもそれを無視し、SEHを常に素通りさせて通常通り
    // クラッシュ(デバッガがその場で停止)させる。デバッグ中でもコンポーネント単位の継続動作
    // 自体を確認したい場合はfalseにする。
    bool IsDebuggerFailFastEnabled();
    void SetDebuggerFailFastEnabled(bool enabled);

    // func を C++例外・SEH例外(nullptr参照、0除算等)の両方から保護して実行する。
    bool SafeExecute(const std::function<void()>& func, std::string& outErrorMessage);
}
