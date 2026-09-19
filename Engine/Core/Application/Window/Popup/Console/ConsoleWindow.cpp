#include "ConsoleWindow.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
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

    struct ConsoleRow
    {
        const NanamiEngine::Module::LogRecord* record;
        int count;
    };

    /** @brief レベルと本文が同じログを、隣接していなくても最初に出た位置の1行へまとめる */
    std::vector<ConsoleRow> CollapseRecords(const std::vector<const NanamiEngine::Module::LogRecord*>& records)
    {
        std::vector<ConsoleRow> rows;
        std::map<std::pair<NanamiEngine::Module::LogLevel, std::string_view>, size_t> rowIndices;
        for (const auto* record : records)
        {
            const auto [iterator, inserted] = rowIndices.try_emplace({ record->level, record->text }, rows.size());
            if (inserted)
                rows.push_back(ConsoleRow{ .record = record, .count = 1 });
            else
                ++rows[iterator->second].count;
        }
        return rows;
    }

    /** @brief 直前の行の、横スクロールしても見えている右端に件数バッジを重ねて描く */
    void DrawCountBadge(const int count)
    {
        const std::string label = std::to_string(count);
        const ImVec2 labelSize = ImGui::CalcTextSize(label.c_str());
        constexpr float paddingX = 6.0f;

        // ContentRegionRect はスクロール分ずれているので、ScrollX を足し戻して見えている右端にする
        const float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x + ImGui::GetScrollX();
        const ImVec2 badgeMin(right - labelSize.x - paddingX * 2.0f, ImGui::GetItemRectMin().y);
        const ImVec2 badgeMax(right, ImGui::GetItemRectMax().y);

        ImVec4 background = ImGui::GetStyleColorVec4(ImGuiCol_Button);
        background.w = 1.0f;
        auto* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(badgeMin, badgeMax, ImGui::GetColorU32(background), (badgeMax.y - badgeMin.y) * 0.5f);
        drawList->AddText(ImVec2(badgeMin.x + paddingX, badgeMin.y), ImGui::GetColorU32(ImGuiCol_Text), label.c_str());
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
    ImGui::Checkbox("Collapse", &collapse_);
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

    if (collapse_)
    {
        for (const auto& row : CollapseRecords(visibleRecords))
        {
            ImGui::TextColored(ColorForLevel(row.record->level), "%s", row.record->text.c_str());
            DrawCountBadge(row.count);
        }
    }
    else
    {
        for (const auto* record : visibleRecords)
        {
            ImGui::TextColored(ColorForLevel(record->level), "%s", record->text.c_str());
        }
    }

    if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();

    return isOpen ? PopupWindowState::Open : PopupWindowState::Closed;
}
