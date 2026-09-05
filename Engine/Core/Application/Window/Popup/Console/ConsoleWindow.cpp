#include "ConsoleWindow.h"

#include <algorithm>
#include <cctype>
#include <ranges>
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

    if (ImGui::Button("Clear"))
    {
        Module::ClearLogHistory();
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
    const std::string searchText = searchBuffer_;

    ImGui::Separator();
    ImGui::BeginChild("ConsoleScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& record : Module::LogHistory())
    {
        if (record.level == Module::LogLevel::Info    && !showInfo_)    continue;
        if (record.level == Module::LogLevel::Warning && !showWarning_) continue;
        if (record.level == Module::LogLevel::Error   && !showError_)   continue;
        if (!searchText.empty() && !ContainsCaseInsensitive(record.text, searchText)) continue;

        ImGui::TextColored(ColorForLevel(record.level), "%s", record.text.c_str());
    }

    if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();

    return isOpen ? PopupWindowState::Open : PopupWindowState::Closed;
}
