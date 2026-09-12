#pragma once
#include <source_location>
#include <string>
#include <vector>

namespace NanamiEngine::Module
{
    enum class LogLevel
    {
        Info,
        Warning,
        Error,
    };

    struct LogRecord
    {
        LogLevel level;
        std::string text;
    };
    
    // NOTE: ログの発生元(ファイル名:行番号)が自動的にtextの先頭へ埋め込まれる。
    void Log       (const std::string& text, std::source_location location = std::source_location::current());
    void LogWarning(const std::string& text, std::source_location location = std::source_location::current());
    void LogError  (const std::string& text, std::source_location location = std::source_location::current());

    // true の場合、LogError() が呼ばれた瞬間にデバッガ(Rider/Visual Studio等)がアタッチされて
    // いれば、その場で __debugbreak() し、実行中のネイティブなコールスタックを確認できるように
    // する。Log()/LogWarning() には影響しない。デフォルトはOFF(通常通りログに出すだけ)。
    bool IsBreakOnLogErrorEnabled();
    void SetBreakOnLogErrorEnabled(bool enabled);

    /** @brief スレッドセーフなログ履歴のスナップショットを返す */
    std::vector<LogRecord> LogHistory();
    /** @brief 保持しているログ履歴をクリアする */
    void ClearLogHistory();
}
