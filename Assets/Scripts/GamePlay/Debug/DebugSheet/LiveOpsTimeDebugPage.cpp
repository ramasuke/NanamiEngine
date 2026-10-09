#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <regex>
#include <string>
#include <vector>

#include "../../../../Data/EventNotice/Data_EventNotice.h"
#include "../../../Core/Game/Condition/Condition_BoardTime.h"
#include "../../../Core/Game/Condition/Condition_Clock.h"
#include "Engine/Core/Application/ApplicationBase.h"
#include "Engine/Core/Object/Registry/ObjectRegistry.h"

namespace GamePlay::Debug
{
    namespace
    {
        namespace Clock = GameCore::Condition::Clock;

        // NOTE: .meta から guid_ を抜き出す。読めなければ空
        std::string ReadMetaGuid(const std::filesystem::path& metaPath)
        {
            std::ifstream stream(metaPath, std::ios::binary);
            const std::string text{ std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };

            static const std::regex GUID_PATTERN(R"re("guid_"\s*:\s*\{[^}]*?"value_"\s*:\s*"([0-9A-Fa-f-]+)")re");
            std::smatch match;
            return std::regex_search(text, match, GUID_PATTERN) ? match[1].str() : std::string();
        }

        // NOTE: .eventNotice を全部集める。開始の早い順
        // NOTE: 型でアセットを列挙する API が無いので、.meta の guid から実体を引く
        std::vector<std::shared_ptr<NanamiEngine::Module::Asset::EventNotice>> CollectEventNotices()
        {
            namespace fs = std::filesystem;
            using NanamiEngine::Module::Asset::EventNotice;

            const std::string metaSuffix = std::string(NanamiEngine::Module::Asset::EVENT_NOTICE_EXTENSION_LABEL) + ".meta";
            std::vector<std::shared_ptr<EventNotice>> notices;
            std::error_code error;
            for (const auto& entry : fs::recursive_directory_iterator("Assets", error))
            {
                if (!entry.is_regular_file() || !entry.path().generic_string().ends_with(metaSuffix))
                    continue;

                const std::string guid = ReadMetaGuid(entry.path());
                if (guid.empty())
                    continue;

                if (const auto notice = NanamiEngine::Core::Application::ApplicationBase::ObjectRegistry()
                    .Catch<EventNotice>(Guid(guid)).lock())
                    notices.push_back(notice);
            }

            std::ranges::sort(notices, [](const auto& a, const auto& b)
            {
                return a->StartTime() < b->StartTime();
            });
            return notices;
        }

        // NOTE: データと同じ "YYYY-MM-DD HH:MM"(日本時間) で出す
        std::string FormatBoardTime(const std::chrono::sys_seconds time)
        {
            const auto local = time + GameCore::Condition::BOARD_TIME_UTC_OFFSET;
            const auto days  = std::chrono::floor<std::chrono::days>(local);
            const std::chrono::year_month_day date{ days };
            const std::chrono::hh_mm_ss timeOfDay{ local - days };

            char text[32];
            std::snprintf(text, sizeof(text), "%04d-%02u-%02u %02d:%02d",
                static_cast<int>(date.year()), static_cast<unsigned>(date.month()), static_cast<unsigned>(date.day()),
                static_cast<int>(timeOfDay.hours().count()), static_cast<int>(timeOfDay.minutes().count()));
            return text;
        }

        const char* NoticeState(const NanamiEngine::Module::Asset::EventNotice& notice, const std::chrono::sys_seconds now)
        {
            const auto start = notice.StartTime();
            const auto end   = notice.EndTime();
            if (!start || !end)  return "期間が読めない";
            if (now < *start)    return "開催前";
            if (now < *end)      return "開催中";
            return "終了";
        }

        void DrawStatus()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            Widgets::Label("実際の時刻", FormatBoardTime(Clock::RealNow()));
            Widgets::Label("ゲームの時刻", FormatBoardTime(Clock::Now()));
            Widgets::Label("ずらし", Clock::IsDebugNowActive() ? "ずらしている" : "なし");
            if (Widgets::Button("実際の時刻に戻す"))
                Clock::ResetDebugNow();
        }

        void DrawSetTime()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;
            using namespace std::chrono_literals;

            Widgets::Header("時刻を指定 (日本時間)");
            static char input[32] = "";
            ImGui::InputTextWithHint("##LiveOpsTime", "YYYY-MM-DD HH:MM", input, sizeof(input));
            GameCore::Condition::DrawBoardTimeWarning(input);
            if (Widgets::Button("この時刻にする"))
            {
                if (const auto target = GameCore::Condition::ParseBoardTime(input))
                    Clock::SetDebugNow(*target);
            }
            if (Widgets::Button("ゲームの時刻を入力欄に写す"))
                std::snprintf(input, sizeof(input), "%s", FormatBoardTime(Clock::Now()).c_str());

            constexpr std::chrono::seconds SHIFTS[] = { -24h, -1h, 1h, 24h, 24h * 7 };
            const int pressed = Widgets::ButtonRow("ずらす", { "-1日", "-1時間", "+1時間", "+1日", "+7日" });
            if (pressed >= 0)
                Clock::ShiftDebugNow(SHIFTS[pressed]);
        }

        void DrawEventNotices()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;
            using namespace std::chrono_literals;

            static std::vector<std::shared_ptr<NanamiEngine::Module::Asset::EventNotice>> notices;
            static bool isCollected = false;
            if (!isCollected)
            {
                notices     = CollectEventNotices();
                isCollected = true;
            }

            Widgets::Header("イベントに合わせる");
            if (notices.empty())
                Widgets::Note("イベント告知が見つからない");

            const auto now = Clock::Now();
            ImGui::PushID("LiveOpsEvents");
            for (const auto& notice : notices)
            {
                const auto start = notice->StartTime();
                const auto end   = notice->EndTime();
                ImGui::PushID(notice.get());
                Widgets::Label(notice->Title(), NoticeState(*notice, now));
                if (start && end)
                {
                    Widgets::Note(FormatBoardTime(*start) + " 〜 " + FormatBoardTime(*end));
                    const int pressed = Widgets::ButtonRow("合わせる", { "開始1分前", "開催中", "終了1分前", "終了後" });
                    if (pressed == 0) Clock::SetDebugNow(*start - 1min);
                    if (pressed == 1) Clock::SetDebugNow(*start + 1min);
                    if (pressed == 2) Clock::SetDebugNow(*end - 1min);
                    if (pressed == 3) Clock::SetDebugNow(*end + 1min);
                }
                ImGui::PopID();
            }
            ImGui::PopID();

            if (Widgets::Button("イベント告知を探し直す"))
                isCollected = false;
        }

        void DrawLiveOpsTime()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            Widgets::Note("開催期間の判定に使う時刻をずらす。ずらした後も時計は進む。");
            Widgets::Note("この PC だけに効き、起動し直すと戻る。マルチプレイの相手には効かない。");
            Widgets::Note("掲示板・ステージ選択は開き直すと変わる。");

            DrawStatus();
            DrawSetTime();
            DrawEventNotices();
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(LiveOpsTime, "時間/開催日時", 61, GamePlay::Debug::DrawLiveOpsTime)
#endif
