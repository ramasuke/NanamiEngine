#include "Story_StoryProgress.h"

#include <filesystem>

#include "../MainProgression/MainProgression.h"
#include "Engine/Module/LocalPrefs/Engine_Module_LocalPrefs.h"
#include "Engine/Module/LocalPrefs/Editor/Engine_Module_LocalPrefs_Editor_ToolBar.h"

namespace GameCore::Story
{
    namespace
    {
        template<class Enum, std::size_t N>
        void DrawCheckboxes(std::set<Enum>& values, const std::array<Enum, N>& items, std::string_view (*toString)(Enum))
        {
            for (const auto item : items)
            {
                bool isOn = values.contains(item);
                if (!ImGui::Checkbox(toString(item).data(), &isOn))
                    continue;

                if (isOn) values.insert(item);
                else      values.erase(item);
            }
        }
    }

    void StorySaveData::OnDrawGui()
    {
        ImGui::SeparatorText("flags");
        DrawCheckboxes(flags, STORY_FLAGS, ToString);
        ImGui::SeparatorText("restoredFacilities");
        DrawCheckboxes(restoredFacilities, FACILITIES, ToString);
    }

    StoryProgress::StoryProgress()
    {
        Reload();
    }

    void StoryProgress::Reload()
    {
        namespace LocalPrefs = NanamiEngine::Module::LocalPrefs;

        const bool hasSave = std::filesystem::exists(
            LocalPrefs::BuildPath(STORY_PROGRESS_SAVE_FILE_PATH, STORY_PROGRESS_SAVE_FILE_KEY));
        data_ = LocalPrefs::LoadOrDefaultWithPath(STORY_PROGRESS_SAVE_FILE_PATH, STORY_PROGRESS_SAVE_FILE_KEY, StorySaveData());

        // NOTE: これより前のセーブには GameProgresion しか無いので、序章を終えていたらそれを引き継ぐ
        if (!hasSave && LoadGameProgression() != GameProgresion::FirstTouchDownMainIsLand)
            data_.flags.insert(StoryFlag::PrologueCleared);

        onChanged_.OnNext(NanamiEngine::R4::Unit{});
    }

    bool StoryProgress::IsSet(const StoryFlag flag) const
    {
        return data_.flags.contains(flag);
    }

    bool StoryProgress::Set(const StoryFlag flag)
    {
        if (!data_.flags.insert(flag).second)
            return false;

        SaveAndNotify();
        return true;
    }

    bool StoryProgress::IsRestored(const Facility facility) const
    {
        return data_.restoredFacilities.contains(facility);
    }

    bool StoryProgress::Restore(const Facility facility)
    {
        if (!data_.restoredFacilities.insert(facility).second)
            return false;

        SaveAndNotify();
        return true;
    }

    void StoryProgress::OnDrawGui()
    {
        if (!ImGui::CollapsingHeader("StoryProgress"))
            return;

        ImGui::PushID("StoryProgress");
        // NOTE: その場で反映して保存する。RestorationGate の見た目もすぐ切り替わる
        const StorySaveData before = data_;
        data_.OnDrawGui();
        if (data_.flags != before.flags || data_.restoredFacilities != before.restoredFacilities)
            SaveAndNotify();

        if (ImGui::SmallButton("Reload"))
            Reload();
        ImGui::PopID();
    }

    void StoryProgress::SaveAndNotify()
    {
        NanamiEngine::Module::LocalPrefs::SaveWithPath(STORY_PROGRESS_SAVE_FILE_PATH, STORY_PROGRESS_SAVE_FILE_KEY, data_);
        onChanged_.OnNext(NanamiEngine::R4::Unit{});
    }

    REGISTER_LOCAL_PREF_WITH_PATH(
        StorySaveData,
        STORY_PROGRESS_SAVE_FILE_KEY,
        StorySaveData(),
        STORY_PROGRESS_SAVE_FILE_PATH)
}
