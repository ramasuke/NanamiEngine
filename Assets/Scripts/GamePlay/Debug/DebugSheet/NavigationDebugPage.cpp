#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include <format>

#include "../../Ui/Navigation/Ui_NavigationMemory.h"

namespace GamePlay::Debug
{
    namespace
    {
        void DrawNavigation()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            auto& memory = Ui::NavigationMemory::Instance();
            Widgets::Note("段を進めるには「ストーリー/フラグ」や「セーブ/クエスト」で状態を変える。");

            Widgets::Header("今の目的");
            const auto& current = memory.Current();
            if (!current)
            {
                Widgets::Note("当てはまる段が無いか、シーンの切り替え中。");
            }
            else
            {
                Widgets::Label("段", current->stepId);
                Widgets::Label("字幕", current->title);
                Widgets::Label("目的地", current->targetId.empty() ? "(このシーンには無い)" : current->targetId);
                Widgets::Label("添え書き", current->label);
                if (const auto position = current->MarkerPosition())
                    Widgets::Label("目印の位置", std::format("{:.1f}, {:.1f}, {:.1f}", position->x, position->y, position->z));
            }

            Widgets::Header("字幕");
            Widgets::Label("最後に出した字幕", memory.AnnouncedTitle());
            if (Widgets::Button("今の目的の字幕をもう一度出す"))
                memory.SetAnnouncedTitle("");
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(Navigation, "情報/ナビ", 72, GamePlay::Debug::DrawNavigation)
#endif
