#pragma once
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

    // NOTE: Log/LogWarning/LogError/LogHistory/ClearLogHistory はすべて内部でmutexを
    // 取っているためスレッドセーフ。別スレッド（例: ネットワークスレッド）から呼んでも良い。
    void Log       (const std::string& text);
    void LogWarning(const std::string& text);
    void LogError  (const std::string& text);

    /** @brief スレッドセーフなログ履歴のスナップショットを返す（ConsoleWindow等が使用） */
    std::vector<LogRecord> LogHistory();
    /** @brief 保持しているログ履歴をクリアする（ファイル出力済みの内容は消えない） */
    void ClearLogHistory();
}
