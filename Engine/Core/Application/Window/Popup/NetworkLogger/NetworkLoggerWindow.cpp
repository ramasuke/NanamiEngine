#include "NetworkLoggerWindow.h"

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
#include "../../../../../Module/Log/NanamiEngine_Module_Log.h"

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

    const char* DirectionText(const NanamiEngine::Module::Network::PacketDirection direction)
    {
        return direction == NanamiEngine::Module::Network::PacketDirection::Send ? "Send" : "Recv";
    }

    const char* DeliveryText(const NanamiEngine::Core::Network::DeliveryMode delivery)
    {
        return delivery == NanamiEngine::Core::Network::DeliveryMode::Reliable ? "Reliable" : "Unreliable";
    }

    /** @brief コピー/保存用に、表示中のパケットログ行を連結する */
    std::string BuildLogText(const std::vector<NanamiEngine::Module::Network::PacketLogRecord>& records)
    {
        std::string text;
        std::ostringstream stream;
        for (const auto& record : records)
        {
            stream.str("");
            stream.clear();
            stream << std::fixed << std::setprecision(3) << "[" << record.timestamp << "] "
                   << DirectionText(record.direction) << " " << record.typeName
                   << " (" << static_cast<int>(record.rawType) << ") "
                   << record.byteSize << "B " << DeliveryText(record.delivery) << '\n';
            text += stream.str();
        }
        return text;
    }

    /** @brief 表示中のパケットログをLogs/にタイムスタンプ付きファイル名で保存する */
    void SaveLogToFile(const std::vector<NanamiEngine::Module::Network::PacketLogRecord>& records)
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
            fileNameStream << "NetworkLog_" << std::put_time(&localTm, "%Y%m%d_%H%M%S") << ".txt";
            const fs::path filePath = directory / fileNameStream.str();

            std::ofstream file(filePath, std::ios::out | std::ios::trunc);
            if (!file.is_open())
            {
                NanamiEngine::Module::LogError("NetworkLoggerWindow: ログの保存に失敗しました: " + filePath.string());
                return;
            }
            file << BuildLogText(records);
            file.close();

            NanamiEngine::Module::Log("NetworkLoggerWindow: ログを保存しました: " + filePath.string());
        }
        catch (const std::exception& exception)
        {
            NanamiEngine::Module::LogError("NetworkLoggerWindow: ログの保存に失敗しました: " + std::string(exception.what()));
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

    const std::string searchTextForFilter = searchBuffer_;
    const auto history = Module::Network::PacketLogHistory();
    std::vector<Module::Network::PacketLogRecord> visibleRecords;
    visibleRecords.reserve(history.size());
    for (const auto& record : history)
    {
        if (record.direction == Module::Network::PacketDirection::Send    && !showSend_)    continue;
        if (record.direction == Module::Network::PacketDirection::Receive && !showReceive_) continue;
        if (!searchTextForFilter.empty() && !ContainsCaseInsensitive(record.typeName, searchTextForFilter)) continue;
        visibleRecords.push_back(record);
    }

    if (ImGui::Button("Clear"))
    {
        Module::Network::ClearPacketLogHistory();
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

    ImGui::Separator();
    ImGui::BeginChild("NetworkLoggerScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& record : visibleRecords)
    {
        ImGui::TextColored(ColorForDirection(record.direction), "[%.3f] %s %s (%d) %zuB %s",
            record.timestamp, DirectionText(record.direction), record.typeName.c_str(),
            static_cast<int>(record.rawType), record.byteSize, DeliveryText(record.delivery));
    }

    if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::End();

    return isOpen ? PopupWindowState::Open : PopupWindowState::Closed;
}
