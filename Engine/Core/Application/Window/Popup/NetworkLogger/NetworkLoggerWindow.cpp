#include "NetworkLoggerWindow.h"

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

    ImVec4 ColorForDirection(const NanamiEngine::Module::Network::PacketDirection direction)
    {
        switch (direction)
        {
        case NanamiEngine::Module::Network::PacketDirection::Send:
            return ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
        default:
            return ImVec4(0.6f, 1.0f, 0.6f, 1.0f);
        }
    }
}

int NanamiEngine::Core::PopupWindow::NetworkLoggerWindow::counter_ = 0;

NanamiEngine::Core::PopupWindow::NetworkLoggerWindow::NetworkLoggerWindow()
{
    id_ = counter_++;
}

NanamiEngine::Core::PopupWindow::PopupWindowState NanamiEngine::Core::PopupWindow::NetworkLoggerWindow::OnDraw(PopupWindowDrawGuiContext context)
{
    bool isOpen = true;
    ImGui::Begin(("NetworkLogger##" + std::to_string(id_)).c_str(), &isOpen);

    if (ImGui::Button("Clear"))
    {
        Module::Network::ClearPacketLogHistory();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Send", &showSend_);
    ImGui::SameLine();
    ImGui::Checkbox("Receive", &showReceive_);
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll_);

    // パケット種別名の検索ボックス
    const bool hasSearchText = searchBuffer_[0] != '\0';
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (hasSearchText ? 55.0f : 0.0f));
    ImGui::InputTextWithHint("##NetworkLoggerSearch", "Search...", searchBuffer_, sizeof(searchBuffer_));
    if (hasSearchText)
    {
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear##NetworkLoggerSearch"))
        {
            searchBuffer_[0] = '\0';
        }
    }
    const std::string searchText = searchBuffer_;

    ImGui::Separator();
    ImGui::BeginChild("NetworkLoggerScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& record : Module::Network::PacketLogHistory())
    {
        if (record.direction == Module::Network::PacketDirection::Send    && !showSend_)    continue;
        if (record.direction == Module::Network::PacketDirection::Receive && !showReceive_) continue;
        if (!searchText.empty() && !ContainsCaseInsensitive(record.typeName, searchText)) continue;

        const char* directionText = record.direction == Module::Network::PacketDirection::Send ? "Send" : "Recv";
        const char* deliveryText  = record.delivery  == Core::Network::DeliveryMode::Reliable   ? "Reliable" : "Unreliable";

        ImGui::TextColored(ColorForDirection(record.direction), "[%.3f] %s %s (%d) %zuB %s",
            record.timestamp, directionText, record.typeName.c_str(),
            static_cast<int>(record.rawType), record.byteSize, deliveryText);
    }

    if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();

    return isOpen ? PopupWindowState::Open : PopupWindowState::Closed;
}
