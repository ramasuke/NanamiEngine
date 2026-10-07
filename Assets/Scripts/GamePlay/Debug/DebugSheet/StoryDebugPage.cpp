#include "Packages/DebugSheet/DebugSheet.h"

#if NANAMI_DEBUG_SHEET_ENABLED
#include "../../../Core/Game/Story/Story_StoryProgress.h"
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"

namespace GamePlay::Debug
{
    namespace
    {
        namespace Story = GameCore::Story;

        /** @brief 今の StoryProgress の中身。全 enum 値を問い合わせて組み立てる */
        Story::StorySaveData Snapshot(const Story::StoryProgress& story)
        {
            Story::StorySaveData data;
            for (const auto flag : Story::STORY_FLAGS)
            {
                if (story.IsSet(flag))
                    data.flags.insert(flag);
            }
            for (const auto facility : Story::FACILITIES)
            {
                if (story.IsRestored(facility))
                    data.restoredFacilities.insert(facility);
            }
            return data;
        }

        void DrawStoryFlags()
        {
            namespace Widgets = NanamiEngine::DebugSheet::Widgets;

            auto& story = Story::StoryProgress::Instance();
            Widgets::Note("切り替えるとその場で保存される。RestorationGate の見た目もすぐ変わる。");

            // NOTE: ゲーム側に手を入れずに済むよう、保存ファイルを書き換えて StoryProgress に読み直させる (Reload が OnChanged も流す)
            auto data    = Snapshot(story);
            bool changed = false;

            Widgets::Header("StoryFlag");
            ImGui::PushID("Flags");
            for (const auto flag : Story::STORY_FLAGS)
            {
                bool isSet = data.flags.contains(flag);
                if (!Widgets::Toggle(Story::ToString(flag), isSet))
                    continue;

                if (isSet) data.flags.insert(flag);
                else       data.flags.erase(flag);
                changed = true;
            }
            ImGui::PopID();

            Widgets::Header("Facility");
            ImGui::PushID("Facilities");
            for (const auto facility : Story::FACILITIES)
            {
                bool isRestored = data.restoredFacilities.contains(facility);
                if (!Widgets::Toggle(Story::ToString(facility), isRestored))
                    continue;

                if (isRestored) data.restoredFacilities.insert(facility);
                else            data.restoredFacilities.erase(facility);
                changed = true;
            }
            ImGui::PopID();

            if (changed)
            {
                NanamiEngine::Module::LocalPrefs::SaveWithPath(Story::STORY_PROGRESS_SAVE_FILE_PATH, Story::STORY_PROGRESS_SAVE_FILE_KEY, data);
                story.Reload();
            }

            Widgets::Header("ファイル");
            if (Widgets::Button("保存内容を読み直す"))
                story.Reload();
        }
    }
}

REGISTER_DEBUG_SHEET_PAGE(StoryFlags, "ストーリー/フラグ", 10, GamePlay::Debug::DrawStoryFlags)
#endif
