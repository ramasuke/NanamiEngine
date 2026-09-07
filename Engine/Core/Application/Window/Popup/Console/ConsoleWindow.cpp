#include "ConsoleWindow.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ranges>
#include <sstream>
#include <string_view>

#include "ImGuiHelper.h"

namespace
{
    /** @brief haystackにneedleが含まれるか大文字小文字を無視して判定する */
    bool ContainsCaseInsensitive(const std::string_view haystack, const std::string_view needle)
    {
        if (needle.empty())
            return true;

        const auto equalsIgnoreCase = [](const char lhs, const char rhs)
        {
            return std::tolower(static_cast<unsigned char>(lhs)) ==
                   std::tolower(static_cast<unsigned char>(rhs));
        };

        return !std::ranges::search(haystack, needle, equalsIgnoreCase).empty();
    }

    ImVec4 ColorForLevel(const NanamiEngine::Module::LogLevel level)
    {
        switch (level)
        {
        case NanamiEngine::Module::LogLevel::Warning:
            return ImVec4(1.0f, 0.75f, 0.0f, 1.0f);
        case NanamiEngine::Module::LogLevel::Error:
            return ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
        default:
            return ImGui::GetStyleColorVec4(ImGuiCol_Text);
        }
    }

    const char* PrefixForLevel(const NanamiEngine::Module::LogLevel level)
    {
        switch (level)
        {
        case NanamiEngine::Module::LogLevel::Warning:
            return "[Warning] ";
        case NanamiEngine::Module::LogLevel::Error:
            return "[Error] ";
        default:
            return "[Log] ";
        }
    }

    /** @brief 色分けが失われるコピー/保存用に、各行へレベル接頭辞を付けて連結する */
    std::string BuildLogText(const std::vector<const NanamiEngine::Module::LogRecord*>& records)
    {
        std::string text;
        for (const auto* record : records)
        {
            text += PrefixForLevel(record->level);
            text += record->text;
            text += '\n';
        }
        return text;
    }

    /** @brief 表示中のログをLogs/にタイムスタンプ付きファイル名で保存する */
    void SaveLogToFile(const std::vector<const NanamiEngine::Module::LogRecord*>& records)
    {
        namespace fs = std::filesystem;
        try
        {
            const fs::path directory = "Logs";
            fs::create_directories(directory);

            const auto nowTimeT = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::tm localTm{};
            localtime_s(&localTm, &nowTimeT);

            std::ostringstream fileNameStream;
            fileNameStream << "ConsoleLog_" << std::put_time(&localTm, "%Y%m%d_%H%M%S") << ".txt";
            const fs::path filePath = directory / fileNameStream.str();

            std::ofstream file(filePath, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                NanamiEngine::Module::LogError("ConsoleWindow: ログの保存に失敗しました: " + filePath.string());
                return;
            }
            file << BuildLogText(records);
            file.close();

            NanamiEngine::Module::Log("ConsoleWindow: ログを保存しました: " + filePath.string());
        }
        catch (const std::exception& exception)
        {
            NanamiEngine::Module::LogError("ConsoleWindow: ログの保存に失敗しました: " + std::string(exception.what()));
        }
    }
}

int NanamiEngine::Core::PopupWindow::ConsoleWindow::counter_ = 0;

NanamiEngine::Core::PopupWindow::ConsoleWindow::ConsoleWindow()
{
    id_ = counter_++;
}

NanamiEngine::Core::PopupWindow::PopupWindowState NanamiEngine::Core::PopupWindow::ConsoleWindow::OnDraw(PopupWindowDrawGuiContext context)
{
    bool isOpen = true;
    ImGui::Begin(("Console##" + std::to_string(id_)).c_str(), &isOpen);

    // history はvisibleRecordsが指すポインタの寿命を保つため、名前付きローカル変数にする
    const auto history = Module::LogHistory();
    const std::string searchText = searchBuffer_;
    std::vector<const Module::LogRecord*> visibleRecords;
    visibleRecords.reserve(history.size());
    for (const auto& record : history)
    {
        if (record.level == Module::LogLevel::Info    && !showInfo_)    continue;
        if (record.level == Module::LogLevel::Warning && !showWarning_) continue;
        if (record.level == Module::LogLevel::Error   && !showError_)   continue;
        if (!searchText.empty() && !ContainsCaseInsensitive(record.text, searchText)) continue;
        visibleRecords.push_back(&record);
    }

    if (ImGui::Button("Clear"))
    {
        Module::ClearLogHistory();
    }
    ImGui::SameLine();
    if (ImGui::Button("Copy"))
    {
        ImGui::SetClipboardText(BuildLogText(visibleRecords).c_str());
    }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        SaveLogToFile(visibleRecords);
    }
    ImGui::SameLine();
    ImGui::Checkbox("Info", &showInfo_);
    ImGui::SameLine();
    ImGui::Checkbox("Warning", &showWarning_);
    ImGui::SameLine();
    ImGui::Checkbox("Error", &showError_);
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll_);

    // ログ本文の検索ボックス
    const bool hasSearchText = searchBuffer_[0] != '\0';
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (hasSearchText ? 55.0f : 0.0f));
    ImGui::InputTextWithHint("##ConsoleSearch", "Search...", searchBuffer_, sizeof(searchBuffer_));
    if (hasSearchText)
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear##ConsoleSearch"))
        {
            searchBuffer_[0] = '\0';
        }
    }

    ImGui::Separator();
    ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto* record : visibleRecords)
    {
        ImGui::TextColored(ColorForLevel(record->level), "%s", record->text.c_str());
    }

    if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();

    return isOpen ? PopupWindowState::Open : PopupWindowState::Closed;
}
