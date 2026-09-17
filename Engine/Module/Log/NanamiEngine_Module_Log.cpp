#include "NanamiEngine_Module_Log.h"

#include <atomic>
#include <iostream>
#include <__msvc_ostream.hpp>
#include <Windows.h>
#include <deque>
#include <fstream>
#include <mutex>
#include <string_view>

namespace NanamiEngine::Module
{
    namespace
    {
        // ConsoleWindow等に表示するログ履歴の上限件数
        constexpr size_t kMaxLogHistory = 2000;

        std::mutex& LogMutex()
        {
            static std::mutex mutex;
            return mutex;
        }

        std::atomic<bool>& BreakOnLogErrorEnabledFlag()
        {
            static std::atomic enabled{false};
            return enabled;
        }

        std::deque<LogRecord>& LogHistoryBuffer()
        {
            static std::deque<LogRecord> history;
            return history;
        }
        
        std::ofstream& LogFile()
        {
            static std::ofstream file("EngineLog.txt", std::ios::out | std::ios::trunc);
            return file;
        }
        
        std::string FormatLocation(const std::source_location& location)
        {
            const std::string_view fullPath = location.file_name();
            const size_t lastSlash = fullPath.find_last_of("/\\");
            const std::string_view fileName = lastSlash != std::string_view::npos ? fullPath.substr(lastSlash + 1) : fullPath;
            return std::string(fileName) + ":" + std::to_string(location.line());
        }

        size_t LogUtf8SequenceLength(const unsigned char leadByte)
        {
            if (leadByte >= 0xC2 && leadByte <= 0xDF) return 2;
            if (leadByte >= 0xE0 && leadByte <= 0xEF) return 3;
            if (leadByte >= 0xF0 && leadByte <= 0xF4) return 4;
            return 0;
        }

        // UTF-8のリテラルと、OS由来でACPのままの文字列(exception.what()やpath.string())が
        // 1本に連結されて届くため、正しいUTF-8列は残し、それ以外のバイトだけACPとして変換する
        std::string LogNormalizeToUtf8(const std::string& text)
        {
            std::string utf8;
            utf8.reserve(text.size());

            size_t index = 0;
            while (index < text.size())
            {
                const auto byte = static_cast<unsigned char>(text[index]);
                if (byte < 0x80)
                {
                    utf8 += text[index++];
                    continue;
                }

                const size_t utf8Length = LogUtf8SequenceLength(byte);
                if (utf8Length != 0 && index + utf8Length <= text.size() &&
                    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data() + index, static_cast<int>(utf8Length), nullptr, 0) > 0)
                {
                    utf8.append(text, index, utf8Length);
                    index += utf8Length;
                    continue;
                }

                const int acpLength = IsDBCSLeadByte(byte) && index + 1 < text.size() ? 2 : 1;
                wchar_t wide[2];
                const int wideLength = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, text.data() + index, acpLength, wide, 2);
                char converted[8];
                const int convertedLength = wideLength > 0
                    ? WideCharToMultiByte(CP_UTF8, 0, wide, wideLength, converted, sizeof(converted), nullptr, nullptr)
                    : 0;

                if (convertedLength > 0)
                    utf8.append(converted, convertedLength);
                else
                    utf8 += '?';
                index += acpLength;
            }
            return utf8;
        }

        std::wstring LogUtf8ToWide(const std::string& utf8)
        {
            const int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
            std::wstring wide(wideSize, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), wide.data(), wideSize);
            return wide;
        }

        void Record(const LogLevel level, const std::string& prefix, const std::string& text,
                    std::ostream& consoleStream, const std::source_location& location)
        {
            std::lock_guard lock(LogMutex());
            const std::string locatedText = "[" + FormatLocation(location) + "] " + LogNormalizeToUtf8(text);

            consoleStream << locatedText << '\n';
            OutputDebugStringW(LogUtf8ToWide(prefix + locatedText + "\n").c_str());

            if (auto& file = LogFile(); file.is_open())
            {
                file << prefix << locatedText << '\n';
                file.flush();
            }

            auto& history = LogHistoryBuffer();
            history.push_back(LogRecord{ .level = level, .text = locatedText });
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

        if (BreakOnLogErrorEnabledFlag().load(std::memory_order_relaxed) && IsDebuggerPresent())
            __debugbreak();
    }

    bool IsBreakOnLogErrorEnabled()
    {
        return BreakOnLogErrorEnabledFlag().load(std::memory_order_relaxed);
    }

    void SetBreakOnLogErrorEnabled(const bool enabled)
    {
        BreakOnLogErrorEnabledFlag().store(enabled, std::memory_order_relaxed);
    }

    std::vector<LogRecord> LogHistory()
    {
        std::lock_guard lock(LogMutex());
        const auto& history = LogHistoryBuffer();
        return std::vector<LogRecord>(history.begin(), history.end());
    }

    void ClearLogHistory()
    {
        std::scoped_lock lock(LogMutex());
        LogHistoryBuffer().clear();
    }
}
