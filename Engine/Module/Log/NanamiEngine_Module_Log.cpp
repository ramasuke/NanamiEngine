#include "NanamiEngine_Module_Log.h"

#include <iostream>
#include <__msvc_ostream.hpp>
#include <Windows.h>
#include <deque>
#include <fstream>
#include <mutex>

// /SUBSYSTEM:WINDOWS ではコンソールが無く std::cout / std::cerr は誰にも見えないため、
// Visual Studio の出力ウィンドウ（DebugView でも可）にも同じ内容を出す
namespace NanamiEngine::Module
{
    namespace
    {
        // ConsoleWindow等に表示するログ履歴の上限件数（超えた分は古いものから捨てる）
        constexpr size_t kMaxLogHistory = 2000;

        std::mutex& LogMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        std::deque<LogRecord>& LogHistoryBuffer()
        {
            static std::deque<LogRecord> history;
            return history;
        }

        // DxLib自身のLog.txtと名前が衝突しないよう別名でリポジトリルートに書き出す
        std::ofstream& LogFile()
        {
            static std::ofstream file("EngineLog.txt", std::ios::out | std::ios::trunc);
            return file;
        }

        void Record(const LogLevel level, const std::string& prefix, const std::string& text, std::ostream& consoleStream)
        {
            std::lock_guard lock(LogMutex());

            consoleStream << text << '\n';
            OutputDebugStringA((prefix + text + "\n").c_str());

            if (auto& file = LogFile(); file.is_open())
            {
                file << prefix << text << '\n';
                file.flush();
            }

            auto& history = LogHistoryBuffer();
            history.push_back(LogRecord{ level, text });
            if (history.size() > kMaxLogHistory)
                history.pop_front();
        }
    }

    void Log(const std::string& text)
    {
        Record(LogLevel::Info, "[Log] ", text, std::cout);
    }

    void LogWarning(const std::string& text)
    {
        Record(LogLevel::Warning, "[Warning] ", text, std::cout);
    }

    void LogError(const std::string& text)
    {
        Record(LogLevel::Error, "[Error] ", text, std::cerr);
    }

    std::vector<LogRecord> LogHistory()
    {
        std::lock_guard lock(LogMutex());
        const auto& history = LogHistoryBuffer();
        return std::vector<LogRecord>(history.begin(), history.end());
    }

    void ClearLogHistory()
    {
        std::lock_guard lock(LogMutex());
        LogHistoryBuffer().clear();
    }
}
