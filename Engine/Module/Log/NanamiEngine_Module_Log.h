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

    /** @brief スレッドセーフなログ履歴のスナップショットを返す（ConsoleWindow等が使用） */
    std::vector<LogRecord> LogHistory();
    /** @brief 保持しているログ履歴をクリアする（ファイル出力済みの内容は消えない） */
    void ClearLogHistory();
}
