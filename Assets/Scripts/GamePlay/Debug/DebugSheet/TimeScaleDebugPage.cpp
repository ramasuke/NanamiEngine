#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <array>
#include <cstdio>

#include "Engine/Core/Application/Time/Time.h"

namespace GamePlay::Debug
{
    namespace
    {
        constexpr std::array<float, 6> PRESETS { 0.0f, 0.1f, 0.25f, 0.5f, 1.0f, 2.0f };

        void DrawTimeScale()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;
            using NanamiEngine::Time;

            char current[16];
            std::snprintf(current, sizeof(current), "x%.2f", Time::GetTimeScale());
            Widgets::Label("今の倍率", current);

            float timeScale = Time::GetTimeScale();
            if (Widgets::SliderFloat("倍率", timeScale, 0.0f, 4.0f))
                Time::SetTimeScale(timeScale);

            const int pressed = Widgets::ButtonRow("プリセット", { "0", "0.1", "0.25", "0.5", "1", "2" });
            if (pressed >= 0)
                Time::SetTimeScale(PRESETS[pressed]);

            if (Widgets::Button("等速に戻す"))
                Time::SetTimeScale(1.0f);

            Widgets::Note("0 で停止。プレイを終えても倍率は戻らない。");
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(TimeScale, "時間/タイムスケール", 60, GamePlay::Debug::DrawTimeScale)
#endif
