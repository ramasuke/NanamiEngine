#include "NanamiEngine_Module_Log.h"

#include <iostream>
#include <__msvc_ostream.hpp>
#include <Windows.h>
#include <deque>
#include <fstream>
#include <mutex>
#include <string_view>

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

        // フルパスは日本語フォルダ(デスクトップ等)を含み得るので、ファイル名部分だけを使う。
        std::string FormatLocation(const std::source_location& location)
        {
            const std::string_view fullPath = location.file_name();
            const size_t lastSlash = fullPath.find_last_of("/\\");
            const std::string_view fileName = lastSlash != std::string_view::npos ? fullPath.substr(lastSlash + 1) : fullPath;
            return std::string(fileName) + ":" + std::to_string(location.line());
        }

        void Record(const LogLevel level, const std::string& prefix, const std::string& text,
                    std::ostream& consoleStream, const std::source_location& location)
        {
            std::lock_guard lock(LogMutex());
            const std::string locatedText = "[" + FormatLocation(location) + "] " + text;

            consoleStream << locatedText << '\n';
            OutputDebugStringA((prefix + locatedText + "\n").c_str());

            if (auto& file = LogFile(); file.is_open())
            {
                file << prefix << locatedText << '\n';
                file.flush();
            }

            auto& history = LogHistoryBuffer();
            history.push_back(LogRecord{ level, locatedText });
            if (history.size() > kMaxLogHistory)
                history.pop_front();
        }
    }

    void Log(const std::string& text, const std::source_location location)
    {
        Record(LogLevel::Info, "[Log] ", text, std::cout, location);
    }

    void LogWarning(const std::string& text, const std::source_location location)
    {
        Record(LogLevel::Warning, "[Warning] ", text, std::cout, location);
    }

    void LogError(const std::string& text, const std::source_location location)
    {
        Record(LogLevel::Error, "[Error] ", text, std::cerr, location);
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
